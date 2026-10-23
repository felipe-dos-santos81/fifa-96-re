# FU-322 — the natural-pass baseline inversion and the P7/P8 window/lockout design (phase-33 R2-L7)

**Phase/Provenance:** M2 phase-33 recon-ahead wave (Stage-1, **task 1 of 4 — `FU322: the natural-pass baseline inversion design (FU321-L11)`**), 2026-10-22, read-only. Deliverable of `docs/superpowers/plans/2026-10-22-fifa96-m2-phase33-recon-ahead.md:33-46` (Steps 1–5) under `docs/superpowers/specs/2026-10-22-fifa96-m2-phase33-live-record-face-design.md` §2 (the binding starting state), §6 (the acceptance, esp. the baseline-inversion and golden-governance paragraphs), §8 decision 6 (the atomic inversion commit). **No fresh Ghidra read in this slice:** every native fact below is *carried* (each row names the frozen doc/section or the port line that transcribes the native bytes); the program `/FIFA96.EXE` was not consulted, because the slice's claims are (a) the port's natural-pass harness, (b) the sibling witnesses' assert sets, (c) the golden mechanism — all first-hand in the C sources read this session (`file:line` cited). **No probe was run and no number is asserted beyond already-frozen cells:** the post-producer values (the first face-driven event frame, the natural pan's seed/axis/frames/place, the re-valued seam counts, the no-ISO behavior) belong to **FU323** (spec §9 item 2) and are forward-cited as named placeholders; this doc fixes the *form*. No repo file was created or changed except this draft; no git; the ISO untouched (never read for write, never written).

---

## 1. The question, and the one-page answer

> **With the live `+0x43` record face staged (FU319 §5), the undriven natural tape pans — what exactly does the inversion commit do to `tests/test_engine_m2.c`'s three zero-baseline asserts, what replaces them (and from which cells), which assertions must land beside them as the "P7/P8 window/lockout" pair, what is a locked pin vs a recorded cell given FU319 §6's turn-transient/settled-face risk class, and which other tests move in the same commit or sequence after?**

**The answer in one paragraph.** The three asserts at `tests/test_engine_m2.c:3722-3724` (`camera_left_zero == 0`, `camera_zero_frames == granted_frames`, `first_left_zero_step == 0`) are the phase-30 NO-GO witness and are replaced — never deleted — by an inverted set that pins the **one-shot pan window** the live face opens on the undriven `M2_KEYS` tape: (A1) `camera_left_zero >= 1` (the pan is the feature), (A2) `camera_zero_frames == first_left_zero_f` (the all-zero frames are *exactly* the pre-event prefix — an identity between two sampled cells that can never re-pin on an unrelated change), (A3) `first_left_zero_step > phase2_step` plus a **bounded window pin** whose two constants are FU323's measured first-event step ± a declared slack, (A4) the window: one contiguous delta run (`segments == 1`) whose bind→motion order (`first_motion_f == first_bind_f + 1`, the settled E14 frame order), run length, stop place and decay chain are asserted **as the `T(s)` formula from the seed the pass itself recorded** (never as literals), and (A5) the lockout: the row-05 one-shot (`post_event_row04_frames == 0`, the event's code `5`), `y_nonzero_frames == 0` (locked — the height-0 idle ramp is a pan law, face-independent), `goal_armed_any == 0`, `oob_frames == 0`, `follow` still waived (`ev == 0`, `follow == 0` on every trace line — the height-0 event cannot open the follow), and the natural gate's **offset pin** (`lane_x >= 1` — the same `G3` form the offset pass pins, because the natural pan *is* the robust non-zero-delta route FU314-L1a wanted). The pan record's identity is pinned once (`first_bind_id == M2_NAT_BAND_ENTITY`, FU323's value) because the two-frame gate ring must name its record; its P3 sensitivity is declared. Recorded-only (never asserted): the exact first-event step inside the window, the seed tuple itself, the stop place as a literal, the settled faces, the per-record turn transients, the band-frame counts — the last two are exactly the class an unrelated change (the separately-pinned `move_attr` metric sub-change, FU319-L3) moves, so asserting them would be a permanent re-pin trap. The commit is **one atomic commit** (FU321-L11, spec decision 6): the inversion, the A4/A5 pair, the golden re-pin with the house-form frame diff, the rewritten site comments/labels, and the moved sibling asserts (the claim/offset/scripted-witness re-pins and the additivity gate's disposition) — with FU323's measured values as the only permitted fill. **GO on the design; every numeric fill is conditional on FU323, and any diff that starts before the first face-driven event or moves a frame no face-gate reader can explain is a stop-and-report, not a re-pin.**

Corollary worth stating up front: the replacement must not be a **golden-only** claim (the plan's review-focus 3) — every replacement assert is computed inside `run_natural_pass` from that run's own sampler, so `make check` catches a dead or wandering pan even if someone hand-edits the golden; and it must not be asserted **from the driven witnesses** — the claim/offset passes re-pin their own tuples and are never read as the natural window's source.

---

## 2. The evidence table

| # | Claim | Evidence (fresh file:line this session, or the frozen doc/section carried) |
|---|---|---|
| E1 | The natural pass replays a **copy** of `M2_KEYS` verbatim (never the golden producer's array), 12 000 steps, no directives, the observation seam installed at match start | `tests/test_engine_m2.c:3630-3643` (`:3638` the probe-local copy), `:1860` (`M2_NATURAL_WINDOW_STEPS 12000`), `:3663-3668`, `:3673-3676` (the seam install) |
| E2 | `M2_KEYS` is a 1800-entry tape (its last entry is index 1799; the phase-2 entry lands at step 1802, after the tape's end) | `tests/test_engine_m2.c:1977-2146` (`:2140-2145` the last block), `:2147` (`M2_KEYS_LEN`); the cap `:1859-1860` |
| E3 | The sampler records the 16 camera cells per granted frame into the aggregate cells `camera_zero_frames` / `camera_left_zero` / `first_left_zero_step` / `first_left_zero_pos` / `sample_hash` | `tests/test_engine_m2.c:3585-3628` (the zero test `:3592-3597`, the counters `:3598-3609`, the hash `:3610-3627`); the struct cells `:2454-2461` |
| E4 | The zero baseline is asserted in-test (not only in the golden), with the site's own forward note | `tests/test_engine_m2.c:3718-3721` (the note, quoting FU313-L1), `:3722-3724` (the three asserts) |
| E5 | The structural window asserts around them (granted 3 598, sampled == granted, the kick/act-1/phase-2 caps) | `tests/test_engine_m2.c:3706-3717` |
| E6 | The seam counters (`calls == 6118`, `select_ran == 0`) are asserted **in both ISO modes**; the AX prepass refuses when `!(si < 0x420 && |cam_z| > 0x7B0)` | `tests/test_engine_m2.c:3725-3731`; `src/fifa96_loader/fifa96_keeper_cpu.c:159-175` (`:165` the gate) |
| E7 | P6/P7/P8 meanings and the RED-by-design comment/label | `tests/test_engine_m2.c:2293-2296`; FU313 §5.3 (`docs/ghidra/FU313_r2l5_natural_acceptance.md:199-201`: P6 leaves zero, P7 the sustained follow window, P8 the lockout); the label `tests/test_engine_m2.c:6049-6057` |
| E8 | P5 determinism (two passes, whole-struct `memcmp`) and the P10 trace self-check (one more natural pass under the sink) | `tests/test_engine_m2.c:4587-4593`, `:3845-3902` (`:3852` the zero-baseline sentence, `:3899-3901` the asserts) |
| E9 | The claim pass (T1) pins its driven tuple: gate `f=1512` lane 0/0 `pos (0,0,0)` `face7d 0x100`, event `f=1513` vel `(−5,0)`, claim `f=1514`, the 27-frame delta run to `−0x90`, `row04_after_event_frames == 0`, `trace_follow1 == 0` | `tests/test_engine_m2.c:4122-4192` (`:4140-4141` pos/face, `:4145` the fallback hazard, `:4175-4176` the off-zero identity, `:4190` the one-shot), P16 `:4200-4268` |
| E10 | The offset pass (FU320) pins its driven tuple: gate `f=1528` `lane_x 36`/`bound 36`/`lane_z 32`/`cam_dz6f −16` `face7d 0x100`, bearing `0x14C`, Δ `0x4C`; event `f=1529`; claim `f=1530`; the 27-frame run to `−0x90` | `tests/test_engine_m2.c:4387-4492` (`:4397-4398` the offset pin, `:4409-4415` the tuple, `:4440` Δ ≤ 0xAB, `:4463-4485` the run), the trace cross-check `:4494-4566` |
| E11 | The scripted pan witness pins the copied-tape chain: `first_tracked == 9`, stops `0x90`/`0x120`, decay `5→3→2→1`, Y/Z `0`, monotone; isolated by `act.id = 0` | `tests/test_engine_m2.c:3745-3843` (the tail overwrite `:3753-3760`, the isolation `:3794`, the asserts `:3826-3840`); `begin_isolated` `tests/test_engine_match_frame.c:157-160` |
| E12 | The m-42 band staging (inside the golden run's directives) stages the camera park, the record placement, `face7d = 0` and `vel73/vel75 = 0` as **declared producer inputs** | `tests/test_engine_m2.c:2793-2889` (`:2842-2846` the park, `:2863-2872` the placement/lane/bound, `:2879` face7d, `:2886-2887` vel73/75) |
| E13 | The m-58/m-61 asserts pin the staged pan in flight (`pos_z 0xB28`, `vel_z 5`, `goal_armed`, the snapshot) and the fifth pan body (`pos_z 0xB32`) | `tests/test_engine_m2.c:2890-2976` (`:2908-2914`), `:3018-3054` (`:3038-3039`); the m-72 final `:3055-3093`; main's anchors `:6218-6225` |
| E14 | The golden transcript is one `frame=<n> hash=<hex> state=<phase>/<score>` line per engine step; the additivity gate pins the v21 2522-line prefix; then the whole-transcript `memcmp` with `report_diff` | `tests/test_engine_m2.c:3269-3271` (the line), `:2513` (the suffix), `:6431-6451` (the gate, `:6438` the 2522 assert), `:6452-6460`; `:1741` (the path); the state-tag asserts `:6400-6407` |
| E15 | The mover's staged defaults are `+0x43` direct-face `0` and the focus point `(0,0)`; the turn twin; the `row44` landing sites the `row43` latch sits beside | `src/fifa96_engine/fifa96_match_run.c:498` (the staged defaults), `:504-548` (`:517` `s.facing`, `:532` the write-back), `:521-522` (the staged metric); `:1029`/`:1067` (the install/successor `row44`); `src/fifa96_engine/fifa96_match_entities.c:565-568` (the kickoff narrow install); `include/fifa96_engine/fifa96_match_entities.h:98/:165/:167` |
| E16 | The follow is refused while `pan_counter != 0`, `follow_speed == 0`, or `event_param <= 0x10`; the height-0 row-04 event keeps `event_param == 0`, so the follow stays closed on the natural tape | `src/fifa96_engine/fifa96_match_run.c:3974-3977`; the height-0 event arm carried FU321 E1/E2 (`handlers.c:2785-2787`, `camera.c:753-762`); the waiver FU317 (`docs/ghidra/FU317_r2l6_claim_acceptance.md:339`); the m-58 dormancy assert `tests/test_engine_m2.c:3006-3007` + main `:6211` |
| E17 | The predicted natural-tape numbers (carried, staged-face probe build): all 22 records `face7d 0x100` over the window; the band record entity 12 settles at face `0x300` = the camera bearing, Δ `0` on 2 082/2 082 band frames, the focus branch on all of them; entity 2 never in the band, 3 turn frames; 4 140/67 320 turn frames (6.1 %), 3..275 per record, 20 distinct settled faces, **none `0x100`**; `is_ball_track` 0 for id 12 (arm B allowed) | `docs/ghidra/FU319_r2l6_direct_face.md` §6 (the three tables) — the wave **re-measures** all of it (spec §6; FU323); treated here as prediction, not binding numbers |
| E18 | The `row43` producer census: three writers (`0x6E6C4` install, `0x8DFBA` successor twin, `0x79C13` kickoff narrow) + the one clear (`0x8540C`), read only by the turn twins (`0x7BF89`/`0x8E29C`); the value is `2` for every record after kickoff | FU319 §2/§5/§8; the port landing sites E15 |
| E19 | The binding cites' line drift: the wave docs' `:3701-3707` baseline and `:4007-4263` T1 pass are pre-shift; fresh `:3722-3724` / `:4031-4269` (T1's earlier insertions moved them; no claim moved) | `docs/ENGINE.md:6480-6483` (erratum 3), `:6626-6631` (erratum 7), `:6656-6664` (the code-review erratum 1) |
| E20 | The FU321-L11 rule and the spec's atomic-commit decision | `docs/ghidra/FU321_r2l6_nonzero_acceptance.md` §5.4 item 7, §8 leg 11, §9 (the Case-B gate); spec §3 (in-slice) + §8 decision 6 |
| E21 | The phase-30 move precedent (the house-form frame diff) | `tests/test_engine_m2.c:1697-1707` (quoted in FU321 §5.2); the field table FU321 §5.2 |
| E22 | The row-04/match_frame fixtures stage their own subjects via `fifa96_match_entities_install(…, 4, …)` (the row-4 body, not the locomotion row) and isolate the non-subjects with `skip_9a` | `tests/test_engine_match_frame.c:7552`, `:7624`, the isolation `:7573-7575`/`:7641-7643`; the face trio `:7651-7715`; `:7725-7771`; `:7779-7824` |
| E23 | The goal-chain fixtures play the chain (single-frame link asserts) and do **not** isolate the non-subject records | `tests/test_engine_match_frame.c:7842-7931` (the links `:7897-7928`), `:7939-7993` |
| E24 | The FU313 gating rule: P6–P9 ISO-gated, P1–P5 both modes — but the zero asserts as landed are **unconditional** (both modes run them) | FU313 `:205` (the rule); `tests/test_engine_m2.c:3706-3731` (no `with_iso` gate around `:3722-3724`) |
| E25 | The degraded no-ISO seed leaves the record positions at zero (all records at the camera, lane 0) and the install-rewritten velocity words give the zero-seed event class | `tests/test_engine_m2.c:3291-3299` (the ISO-gated position asserts), the degraded-seed comment `:3290-3292`; the zero-seed event FU321 §6.8/leg L2 |
| E26 | The census holds `60/80 = 75%`; the four-doc same-commit rule is not triggered by a test/golden-only change | `docs/ENGINE.md:6555-6557`, FU321 §5.5; AGENTS.md's derived-percentage rule |
| E27 | M1 is out of the face's reach: its tape is boot → intro → front-end → **QUIT** (no MATCH mode is ever entered) | `tests/test_engine_m1.c:1-25` (the header), `:38` (`M1_STEP_CAP 5000`), `:45-52` (the nav keys) |

---

## 3. What the harness pins today (the end-to-end read)

### 3.1 The run and its sampler

`run_natural_pass` (`tests/test_engine_m2.c:3634`) boots the engine on the null platform with a **copy** of `M2_KEYS` (`:3638` — the 1800-entry tape of `:1977-2146`; `M2_KEYS_LEN` `:2147`) and runs `M2_NATURAL_WINDOW_STEPS = 12 000` live steps (`:1860`, the loop `:3663-3705`). At match start it installs the FU282 observation seam (`:3673-3676` — stage-only, FU313 §5.2.4). Each granted frame (a `pace.pending` delta, `:3696-3702`) calls `m2_natural_sample` (`:3701`), which (a) tests all **16 camera cells** for the all-zero state (`pos/vel/speed/follow_speed/event_param/event_cursor/tracked/pan_counter/timer/timer_limit/ramp_divisor/rate_x/rate_z` — `:3592-3597`), (b) counts `camera_zero_frames` / `camera_left_zero` and records the first departure's step and pos triple (`:3598-3609`; the cells `:2454-2459`), and (c) folds the 16 cells + phase + clock into the P5 `sample_hash` (`:3610-3627`). The tail asserts pin the structural window (match start 5, the kick/act-1/phase-2 ordering and caps, `granted_frames == 3598`, `sampled_frames == granted_frames` — `:3706-3717`), the zero baseline (`:3722-3724`), and the seam (`:3728-3731`).

### 3.2 What P6/P7/P8 mean today

- **P6 — the camera leaves zero**: the natural-window witness, carried by `camera_left_zero`; the scripted claim/kick witness on a *copied* tape with the cutscene isolated is labelled **P6 partial** (`:2347-2355`, `:3736-3744`) because it is scripted, one-shot, X-only and not natural open play.
- **P7 — the sustained follow window**: a contiguous `follow == 1 && pan == 0` run with `vel == dirs*15`, its length `ramp_divisor/2` at the bind (FU313 §5.3 `:200`, mirroring `tests/test_engine_match_frame.c:2490-2515`).
- **P8 — the lockout**: after that window `pan_counter == 1` and `follow == 0` until a new setter re-opens (FU313 §5.3 `:201`, `tests/test_engine_match_frame.c:2516-2523`).
- Today all three are **RED by design in the natural window** (`:2293-2296`: "the camera never leaves (0,0,0) … the probe records it, it does not hide it"), and the stderr label says exactly that (`:6049-6057`: "P6 partial …; P7/P8 RED by design — no camera cell leaves (0,0,0) and no follow window/lockout exists over the natural window (the phase-30 NO-GO baseline)").

### 3.3 The two traps the harness already documents

1. The zero baseline is asserted **unconditionally** (both ISO modes run `:3722-3724`; E24/E25) — so a producer that flips the ISO-mode window also flips (or must be argued not to flip) the degraded no-ISO window, whose record positions all sit at zero (`:3291-3295`).
2. The `zero_pos` pin in the P10 self-check (`:3900`, the sentence `:3852`) is a **second** zero pin over the same run — it moves with the inversion too.

### 3.4 The sibling assert sets that share the tape

- The **claim pass** (`run_natural_claim_pass`, `:4031`): `M2_KEYS` + a DOWN hold from tape index 1820 (`:1869`, `:4051-4056`); pins the zero-delta gate tuple at `f=1512` — `lane_x 0`/`lane_z 0`/`cam_dz6f 0`/`pos (0,0,0)`/`face7d 0x100` (`:4131-4145`) — the event/claim/pan timeline (`:4161-4192`), the off-zero identity (`:4175-4176`), and the P16 trace cross-check (`:4200-4268`).
- The **offset pass** (`run_natural_offset_pass`, `:4285`): `M2_KEYS` + LEFT 1820..1849 + DOWN 1850.. (`:1884-1888`, `:4308-4321`); pins the FU320 tuple — gate `f=1528`, `lane_x 36`/`bound 36` (the plateau, `:4409-4412`), `face7d 0x100` (`:4415`), bearing `0x14C`/Δ `0x4C` (`:4436-4440`), event `f=1529`, claim `f=1530`, the 27-frame run to `−0x90` (`:4463-4485`).
- The **scripted pan witness** (`run_scripted_pan_witness`, `:3745`): the copied tape + the long-ball/UP tail (`:3753-3760`) with `act.id = 0` (`:3794`); pins `first_tracked == 9`, the `0x90`/`0x120` stops, the `5→3→2→1` decay, Y/Z `0` (`:3826-3840`).

### 3.5 The golden run's own fixtures

The golden transcript is produced by `run_scenario` (`:3214`) replaying `M2_KEYS` **with directives** (`m2_directives`, `:2575`): the forced kickoff windows (m 1/m 21), the mechanics staging (m 41, which **forces phase 2** and stages the wired rows), the **m-42 band scenario** (`:2793-2889` — the camera park `(0,0,0xB00)`, the record placed at lane 0x10 with `face7d = 0` and `vel73/vel75 = 0` as declared producer inputs), the **m-58 goal link** (`:2890-2976` — the staged pan in flight), the scored/final sets (m 61/m 72, `:3018-3093`). The transcript's additivity gate pins the v21 2522-line prefix (`:6431-6451`) before the whole-transcript `memcmp` (`:6452-6460`).

---

## 4. The design — the inverted assert set (the commit's exact content)

**Rule carried verbatim (FU321 §8 leg 11 + §9):** *"the natural-pass baseline asserts flip with a universal producer … the P7/P8 window/lockout assertions must land in the same commit as the inversion, or `make check` stays RED for a legitimate producer"*; §9's Case-B gate: *"deleting or weakening the zero-camera asserts without the replacement window is a NO-GO."* Spec §8 decision 6: *"The inversion commit is atomic: the `:3701-3707` inversion, P7/P8, the golden re-pin, and the node's provenance note in the same commit."*

### 4.1 The replacement for each of the three asserts

All three sit today at `tests/test_engine_m2.c:3722-3724` inside `run_natural_pass`, after the structural block; the replacements occupy the same site. New sampler cells first (§4.3), then the asserts.

**A1 — replaces `assert(nat->camera_left_zero == 0u);` (:3722).**

```c
/* The inverted baseline (FU322/FU321-L11): with the live +0x43 latch the
 * undriven tape pans from the first face-driven event; the phase-30 zero
 * witness is replaced by the one-shot pan window + lockout (A4/A5 below). */
assert(nat->camera_left_zero >= 1u);
```

Locked. This fails only if the natural pan dies (the producer regressed, or a staging suppressed the band) — exactly the regression it exists to catch. It is deliberately *not* an identity: it is the feature claim. **Guard against the zero-seed false pass:** a departure counted by A1 could be a tracked bind with no motion (the FU321-L2 class) — A4's bind→motion identity and `segments == 1` are what make A1 mean a real pan, so A1 is never landed alone.

**A2 — replaces `assert(nat->camera_zero_frames == nat->granted_frames);` (:3723).**

```c
assert(nat->camera_zero_frames == (uint32_t)nat->first_left_zero_f);
```

where `first_left_zero_f` (new, `:2458`'s neighbour) is the 0-based granted-frame index of the first not-all-zero sample. This is an **identity between two cells the same run sampled**: the all-zero frames are exactly the pre-event prefix. It asserts contiguity — any all-zero frame *after* the first departure (e.g. a period reset inside the window) breaks it and demands its own written reason — and it can never re-pin on an unrelated engine change, because both sides move together. The old right-hand side (`granted_frames`) is retained in A5's off-zero identity instead.

**A3 — replaces `assert(nat->first_left_zero_step == 0);` (:3724).**

```c
assert(nat->first_left_zero_step > nat->phase2_step);
assert(nat->first_left_zero_step >= M2_NAT_FACE_FIRST_LO);
assert(nat->first_left_zero_step <= M2_NAT_FACE_FIRST_HI);
```

- `> phase2_step` is locked structure: the row-04 gate's phase term makes any face-driven event strictly after the phase-2 entry (FU314 §6's gate tuple, carried).
- The `M2_NAT_FACE_FIRST_LO/HI` pair is the **window pin**: two named constants defined beside the drive macros (`:1884-1888`'s block) whose values are FU323's measured first-event step ± a declared slack, with the derivation rule written at the definition site (§5, item 3). The exact step stays **recorded** (`first_left_zero_pos` + the stderr line), never hard-pinned — the event frame is face-coupled pacing (the turn transient + the band entry), which the separately-pinned `move_attr` metric sub-change (FU319-L3) moves.

**A4 — the P7-analog window (lands beside A1–A3, same commit).**

```c
assert(nat->pan.segments == 1u);                                /* one contiguous delta run */
assert(nat->pan.first_bind_f == nat->first_left_zero_f);        /* the departure is the bind */
assert(nat->pan.first_motion_f == nat->pan.first_bind_f + 1);   /* E14: the event moves the place next frame */
assert(nat->pan.changed_frames == m2_nat_pan_frames(nat->pan.event.cam_vel_x,
                                                    nat->pan.event.cam_vel_z));
assert(nat->pan.pan_max == m2_nat_pan_steps(same));
assert(nat->pan.stop_x == m2_nat_pan_place_x(same) && nat->pan.stop_y == 0 &&
       nat->pan.stop_z == m2_nat_pan_place_z(same));
```

The seed is **not** asserted; the *arithmetic is*. `m2_nat_pan_*` are three tiny static helpers implementing the frozen closed form (FU321 §3.4 E9: `v0 = trunc(s/0xC)` — taken directly from the event frame's `cam_vel`, so the helpers are v0-determined and arm-agnostic — then `v_{i+1} = trunc(3·v_i/4)`, `frames = 6 + 7(n−1)`, `T = 2·Σ n_i v_i`). The helpers are written against `fifa96_camera.c`'s live rates (`:942-944`, `:770-777`) and cross-checked in the RED→GREEN against the claim/offset passes' measured `(5, 27)`/`(−0x90, 27)` instances, which already pin the same arithmetic (E9/E10). Because the inputs are the run's own recorded cells, the assert holds for **either** seed arm (the slot dir or the no-slot velocity words — FU321 §3.2) and cannot be satisfied by the zero-seed silent event (`|v0| ≥ 1` follows from `speed > 0` at the event, the corrected FU321 criterion).

**A5 — the P8-analog lockout (lands beside A4, same commit).**

```c
assert(nat->pan.off_zero_frames ==
       nat->granted_frames - (uint32_t)nat->pan.first_motion_f);  /* no zero place after the pan */
assert(nat->post_event_row04_frames == 0u);       /* the row-05 one-shot: the panner leaves row 04 */
assert(nat->pan.event.code == 5u);                /* the same-frame coda (the claim-pass precedent, :4162) */
assert(nat->pan.gate.lane_x >= 1);                /* G3/§3.8: the natural pan is a non-zero-delta pan */
assert(nat->pan.gate.lane_z != 0 || nat->pan.gate.cam_dz6f != 0);
assert(nat->y_nonzero_frames == 0u);              /* the height-0 idle ramp is a pan law */
assert(nat->goal_armed_any == 0u && nat->oob_frames == 0u);   /* no pan from the park reaches the bounds */
```

Plus, inside `run_natural_trace_self_check` (the same commit), the trace-side waiver and the replacement for `zero_pos` (`:3900`):

```c
assert(ev == 0);                 /* per body line: the height-0 event keeps ev = 0 */
assert(follow == 0);             /* per body line: the follow stays waived */
assert(zero_pos == body - traced.pan.changed_frames);   /* replaces zero_pos == granted_frames */
```

and `body == traced.granted_frames` / `body == 3598u` (`:3899/:3901`) stay untouched (face-independent structure).

### 4.2 The Y=Z=0 ruling

- **Y survives, locked.** The row-04 half-line arms pass height 0 to the camera event (FU321 E1), height 0 selects the idle ramp with `event_param = 0` (E2), and `pos_y` is written only when `event_param != 0` — so no row-04 pan can move Y on any tape, driven or not (FU321 §3.7). The face producer does not touch the height. `y_nonzero_frames == 0` is therefore a **pan law**, not this drive's property, and is locked in A5.
- **Z does not survive as a literal zero.** The phase-31 `Y=Z=0` was the DOWN drive's property (FU321 §3.7); on the undriven tape the seed decides, and FU319 §6 does not predict the natural seed's axis (the slot arm's dir or the no-slot velocity words — §4.1's arm-agnostic form covers both). The design asserts `stop_z == T(s_z)` **through the formula** and **records** `z_nonzero_frames`; a literal `z_nonzero_frames == 0` is added only in the Stage-2 commit **if FU323's measured seed has `s_z == 0`**, cited at the assert site — never assumed.

### 4.3 The new cells the pass must sample (what makes the replacements assertable)

Added to `struct m2_natural` (`:2447-2466`) and filled by `m2_natural_sample` (`:3589-3628`):

| new cell | type | role |
|---|---|---|
| `first_left_zero_f` | `int` | the 0-based granted-frame index of the first not-all-zero sample (A2/A4's identity left side) |
| `pan.first_bind_f`, `first_bind_id` | `int` ×2 | the first frame with `tracked != NONE` and the bound record (A4's identity; the recorded pan-record identity) |
| `pan.*` (the delta-run set) | the `m2_claim` field set (`segments`, `first_motion_f`, `stop_f`, `changed_frames`, `off_zero_frames`, `stop_x/y/z`, `pan_at_motion`, `pan_max`, `speed_seq[8]`, `speed_seq_len`, `prev_x/y/z`, `have_prev`, `in_segment`) | A4/A5's window/lockout cells — the same counters the claim/offset passes already run, replayed on the undriven run |
| `pan.gate`, `pan.event` | `struct m2_claim_gate` ×2 | the gate/event tuples at the first motion (the ring's `f−2`/`f−1`), with **`vel73`/`vel75` added to `struct m2_claim_gate`** (`:2375-2390`) so the no-slot seed arm is recorded |
| `post_event_row04_frames` | `uint32_t` | frames with the panning record's `code == 4` after the event (the A5 one-shot) |
| `pan_record_code_at_event` | `uint8_t` | the event snapshot's code (the `5` coda assert) |
| `y_nonzero_frames`, `z_nonzero_frames` | `uint32_t` ×2 | the Y/Z counters (A5; the Z record) |
| `goal_armed_any`, `oob_frames` | `uint8_t`/`uint32_t` | the A5 bounds counters (`fifa96_camera_out_of_bounds` / `mr->goal_armed`, as the claim sampler already does at `:4015-4016`) |

Mechanics: `m2_natural_sample` gains (i) the two-frame **named-record ring** — each granted frame snapshots the record named by `M2_NAT_BAND_ENTITY` (a new macro beside `M2_CLAIM_ENTITY` `:1876`, value = FU323's measured band record), exactly the `m2_claim_snapshot` shape but parameterized (a new `m2_nat_gate_snapshot(g, mr, f, id)` twin; the claim/offset passes keep theirs untouched); (ii) the delta-run/segment state machine copied from `m2_claim_sample` (`:3968-3995`); (iii) the counters above. The first bind selects the ring: `first_bind_id` is asserted equal to the macro (the identity that makes the ring's gate tuple the pan's own — the P3 sensitivity is declared at the macro, §5 item 4). The no-ISO branch is §4.4.

### 4.4 The mode gating (the trap §3.3.1)

Today the three zero asserts run in **both** modes (E24). The inverted set is **ISO-gated** (FU313 §5.3's own rule, `:205`: the acceptance asserts are ISO-dependent because the formation seed places the records):

```c
if (with_iso) {
  /* A1..A5 (the inverted baseline + the window/lockout pair) */
} else {
  /* the degraded seed's face-coupled behavior is unmeasured (FU322-L10):
   * camera_zero_frames / camera_left_zero / first_left_zero_step(+pos) /
   * first_bind_f/id are recorded to stderr and asserted by nobody until
   * FU323 measures this mode; the structural P1-P5 and seam asserts stay. */
}
```

The self-check's replacement asserts (`:3900`, the new `ev`/`follow` per-line checks) are gated the same way, because the no-ISO trace's zero_pos may legitimately stay equal (the zero-seed bind-only outcome, E25) or not — unmeasured.

### 4.5 The sibling re-pins (what else in this file the same commit carries)

The live face moves the **driven** tapes too (their gate tuples read the now-live `+0x7D`, and FU319 §6 predicts none of the 22 settled faces is `0x100` anymore). Per FU321-L11's spirit and the spec §3 ("the re-check of the phase-32 driven/claim/offset witnesses under the live face"), the same commit re-pins them from **FU323's measured drive diffs**:

- the claim pass's gate face pin `face7d == 0x100` (`:4141`) and, if the gate's Δ moves out of `≤ 0xAB`, its whole timeline (`:4131-4192`) — possibly demising the zero-delta event entirely (leg FU322-L3);
- the offset pass's tuple (`:4403-4443`, the face `:4415`) and timeline (`:4447-4485`) (leg FU322-L4);
- the scripted witness's `first_tracked == 9` and stop/decay pins (`:3826-3840`) if the natural band pan pre-empts the scripted claim on the copied tape (leg FU322-L5).

The rule for each: **re-pin with the measured value + the frame-diff reason, or re-shape to the measured behavior with the reason quoted — never delete, never silently upgrade.** If FU323 returns a diff these re-pins cannot explain, the commit stops (spec §7).

### 4.6 The comment/label rewrites (the provenance note)

The same commit rewrites, in place, the four texts that declare the old baseline — these are the "node's provenance note" spec decision 6 names:

1. `:2293-2296` — "P6-P8 are RED by design in the natural window…" → the inverted wording: P6 GREEN via the live-face pan; the one-shot window/lockout pinned; the FU313 P7/P8 follow form stays waived.
2. `:3585-3588` — the sampler comment ("`camera_left_zero` is the P6 witness; today it is 0…") → the inverted description + the new cells.
3. `:3718-3721` — the site note ("the bootstrap producer will flip it…") → the inversion's own record: what flipped it (the live `+0x43`), the commit's atomic contents, and the pointer to FU323's measured diff.
4. `:6049-6057` — the stderr honest-baseline label → the post-inversion label, including the retained follow waiver and the pan window's recorded cells.
5. `:3845-3853` — the P10 comment's zero-baseline sentence (`:3852`) → the identity-based wording.

The file-level **provenance block** (the phase-30 house form, E21/FU321 §5.2's field table) is appended after the last existing provenance block with: old→new M2 sha + bytes/lines, the first and last differing line (line number + `frame=` id), the moved `frame=` range, the moved columns (`hash=` only expected; the `state=` suffixes verified unchanged by the cut-diff), the strata (touched/added/removed — a new stratum is a red flag), the anchors crossed (m 41 / the m-42 band / m 58 / the v21 2522-line prefix), the intended consequence (the first face-driven event frame and the pan window, from FU323), and the collateral classification (every other moved frame, or "none").

---

## 5. Locked pin vs recorded cell (per item, with the reason)

The rule applied: **an assert that would re-pin on any unrelated engine change is not an assert** — it is a record. The risk class is FU319 §6's turn-transient/settled-face geometry: 4 140/67 320 turn frames, 3..275 nonzero-turn frames per record, 20 distinct settled faces — values produced by a *pacing* feedback (the `move_attr` metric, the walk history, the AI's face-gated decisions), not by the feature under test.

| # | cell | decision | why |
|---|---|---|---|
| 1 | `camera_left_zero >= 1` | **LOCKED** | the feature claim; re-pins only if the pan dies — the regression the assert exists for |
| 2 | `camera_zero_frames == first_left_zero_f` | **LOCKED (identity)** | both sides are the same run's sampled cells; they move together, so nothing unrelated re-pins it; it catches a post-departure all-zero frame (a reset) as an honest RED |
| 3 | `first_left_zero_step > phase2_step` | **LOCKED** | the gate's phase term is face-independent structure |
| 4 | `first_left_zero_step ∈ [LO, HI]` | **LOCKED WINDOW, exact recorded** | the exact frame is face-coupled *pacing* (the turn transient + the band entry); the window's width covers the declared pacing slack (≥ the `move_attr` sub-change's transient shift, FU319-L3) so the P3 commit is not forced to re-pin the inversion's own assert; the exact value goes to stderr |
| 5 | `first_bind_id == M2_NAT_BAND_ENTITY` | **LOCKED, sensitivity declared** | needed to make the gate ring's tuple the pan's; the band-record identity is walk-driven (stable) but its first-*pass* order is face-coupled — a P3 order-flip is a declared, reasoned re-pin, not a silent trap |
| 6 | `segments == 1` | **LOCKED (from FU323)** | the one-shot; a second segment is a real second pan (the band geometry after the first pan moves the camera) — re-pins only with a face-gate reason, never from pacing noise alone |
| 7 | `first_motion_f == first_bind_f + 1` | **LOCKED** | E14's settled frame order (the camera walk precedes the entity chain) is a design constant; a reorder is a fidelity break worth an RED |
| 8 | the `T(s)` arithmetic (frames/place/decay/`pan_max` from the recorded v0) | **LOCKED as a formula** | holds for any seed arm; re-pins only if the pan arithmetic itself changes — which is what the helpers pin |
| 9 | `stop_y == 0`, `y_nonzero_frames == 0` | **LOCKED** | the height-0 idle ramp is a pan law (FU321 §3.7), face-independent |
| 10 | `stop_z == T(s_z)`, `z_nonzero_frames` | **RECORDED; literal 0 conditional on FU323's seed** | Z is the drive's property, not a pan law (FU321 §3.7); the seed's axis is FU323's |
| 11 | `post_event_row04_frames == 0`, the event's `code == 5` | **LOCKED** | the row-05 one-shot (the coda) — the claim pass already pins the same shape (`:4190`, `:4162`); it is the lockout's content |
| 12 | `goal_armed_any == 0`, `oob_frames == 0` | **LOCKED** | the formula bound: no pan from the origin park reaches the armer/OOB bounds (FU321 §3.7's E24 argument, `0x90 < 0x730/0xB20`) |
| 13 | the trace's `ev == 0`, `follow == 0` per line | **LOCKED** | the height-0 event cannot open the follow (E16); the FU317 waiver stands and is now *asserted* rather than merely waived |
| 14 | the seam `calls` / `select_ran` | **LOCKED, re-valued once from FU323** | the count is AI-coupled (the face-gated decisions can move the keeper route), so FU323's diff re-values it in the commit; `ran == 0` survives structurally (|cam_z| ≤ 0x90 < 0x7B0, `keeper_cpu.c:165`) |
| 15 | the settled faces (20 values), the per-record turn-transient counts (3..275), the band-frame counts (2 082) | **RECORDED ONLY (never asserted)** | the chaos class: produced by pacing feedback, moved by the metric sub-change and any walk/AI change; they are covered implicitly by the golden's hashes (the re-pin's frame diff classifies them) and by FU323's tables — asserting them would re-pin on every unrelated change |
| 16 | the exact first-event step, the seed tuple, the stop place as a literal | **RECORDED (stderr) / formula-asserted** | the values are FU323's; the formula assert (item 8) is the pin that does not depend on them |
| 17 | `granted_frames == 3598`, `sampled_frames == granted`, the phase caps, the self-check's `body == 3598` | **LOCKED (unchanged)** | face-independent structure (the pace cadence and the input machine) |

---

## 6. The commit shape (the atomic inversion commit)

One commit, type `test` (the house list; the golden + the test-side change ride together), containing:

1. `tests/test_engine_m2.c` — the inverted baseline A1–A3, the A4/A5 window/lockout pair, the new sampler cells and helpers (§4.3), the mode gating (§4.4), the sibling re-pins per FU323 (§4.5), the comment/label rewrites (§4.6), and the additivity-gate disposition (below).
2. `tests/golden/engine/m2-frames.txt` — the re-pin, in this exact order (FU321 §5.3's commands, quoted for the Stage-2 plan):
   ```
   ./build/test_engine_m2 > /tmp/opencode/fu322-m2-new.txt        # ISO present, CWD = repo root
   cmp tests/golden/engine/m2-frames.txt /tmp/opencode/fu322-m2-new.txt   # the first diff
   # analyse per the house form (§4.6); only if the diff is the intended consequence:
   ./build/test_engine_m2 > tests/golden/engine/m2-frames.txt
   sha256sum tests/golden/engine/m2-frames.txt tests/golden/engine/m1-frames.txt
   ./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt       # M1 must not move
   make check
   ```
3. `docs/ENGINE.md` — the register pin (the phase-33 P2 entry: the shas, bytes/lines, the frames, the reason — the house form).
4. `docs/PROGRESS.md` — if the slice closes.
5. README/AGENTS — untouched: the census holds `60/80 = 75%` (no federated action-row wiring; E26).

**The additivity gate's disposition (FU322-L7, decided by FU323's first differing golden line):**

- **Outcome A — the first differing line is after the v21 2522-line prefix:** the gate (`:6431-6451`) survives untouched; the register notes the prefix held.
- **Outcome B — the first differing line is inside the prefix:** the byte-prefix assert is **replaced** in the same commit by the automated **stratum-set gate**: collect the transcript's distinct `state=` tags and assert the set equals the registered one (the `:6400-6407` tags, enumerated as a table) — the durable form of the "no new stratum is a red flag" rule, robust to hash moves. The byte-prefix form is incompatible with a mid-transcript re-pin and is tautological post-re-pin (FU321-L6), which is the quoted reason; the replacement is the stratum gate + the per-frame state-column check in the procedure (`diff <(cut -d' ' -f3- …)`). Deleting the prefix gate without the stratum replacement is the FU321-L11 NO-GO class.

**Golden governance interface (owned by FU323/FU325, not this doc):** the spec §6 rule — the first differing frame at or after the first face-driven event; any earlier diff or an unexplainable frame is a stop-and-report. In the golden run the "first face-driven event" is the m-42 staged dispatch's face-gated pass (whose face operand is the staged producer input, E12) — whether the governance anchor is that frame or the undriven-tape event is FU325's ruling from FU323's diff; this doc does not pre-rule it (leg FU322-L6).

---

## 7. The hazards (which other tests read the natural window or pin camera zero)

**Same commit (they read the inverted run or break under the producer — `make check` must be green in the one commit):**

| # | site | what pins | why it moves |
|---|---|---|---|
| H1 | `tests/test_engine_m2.c:3900` (+ the sentence `:3852`) | `zero_pos == traced.granted_frames` over a traced natural pass | the same run's second zero pin; replaced by the identity (§4.1) |
| H2 | `tests/test_engine_m2.c:4131-4192` (esp. `:4140-4141`, `:4145`) | the claim pass's zero-delta gate tuple (pos, face 0x100, lane 0/0) | the gate tuple is face-coupled: with a settled face ≠ 0x100 the Δ = fold(0x80 − face) can leave `≤ 0xAB` and demises the T1 event (FU322-L3) |
| H3 | `tests/test_engine_m2.c:4403-4492` (esp. `:4409-4415`, `:4440`) | the offset pass's plateau tuple (lane/bound 36/36, face 0x100, Δ 0x4C) | the same face coupling; the plateau geometry is FU320-L1's known fragility (FU322-L4) |
| H4 | `tests/test_engine_m2.c:3826-3840` | the scripted witness (`first_tracked == 9`, the stops, the decay) | the natural band pan can pre-empt the scripted claim on the copied tape (FU322-L5) |
| H5 | `tests/test_engine_m2.c:6035-6048`, `:6049-6057` | the stderr records/label | the provenance wording (§4.6) |
| H6 | `tests/test_engine_m2.c:6431-6451` | the additivity gate's 2522-line prefix | the disposition rule (§6, FU322-L7) |
| H7 | `tests/test_engine_m2.c:3728-3731` | the seam counters | re-valued once from FU323's diff (FU322-L8); `select_ran == 0` survives structurally |

**Sequence after (verified against FU323's measured diff; they move in the same commit only if the live face breaks them — the break→same-commit rule):**

| # | site | what pins | expected and why |
|---|---|---|---|
| H8 | `tests/test_engine_match_frame.c:1114-1140` (`test_natural_phase2_never_scores`) | the natural chain's negative (score 0-0 over 300 phase-2 frames) | survives: the pan from the origin cannot reach the `0xB20` armer (FU321 §3.7), and the fixture pins no camera cell |
| H9 | `tests/test_engine_match_frame.c:2437-2539` | the follow window `== 52` (`:2515`) + the lockout (`:2516-2523`) | survives: the subject is the follow arm — its re-seed reads the slot dirs (re-pinned per frame `:2499-2500`), its gates are height/event-param driven (`:3974-3977`), and the camera Y is pinned (`:2450-2451`); the turn does not enter the chain |
| H10 | `tests/test_engine_match_frame.c:7535-7824` | the row-04 fixtures (the live pan arming, the face-gate trio, the tracked suppression, arm A) | survives: the subjects are installed on **row 4** (the row-04 body never reads `+0x43`; E22), their `face7d` stagings are declared producer inputs (`:7635`), and the non-subjects are `skip_9a`-isolated (`:7573-7575`/`:7641-7643`) |
| H11 | `tests/test_engine_match_frame.c:7842-7993` (`test_natural_goal_end_to_end`, `…_fallback_arm`) | the played goal chain's single-frame links | at risk: the non-subject records are **not** isolated (E23); under the live face the code-3 records (the locomotion-row carriers) turn and the defender decision (`outfield.c:384`, FU321 §5.4 item 5) can re-decide — a timing shift breaks the one-frame links. FU323's diff rules it; a break moves in the inversion commit |
| H12 | `tests/test_engine_m1.c` (the whole binary; `:38`, `:45-52`) | the immutable M1 golden | structurally safe: the tape quits before any MATCH mode is entered (E27), so the live face cannot reach it; the `cmp` runs in the governance anyway and a move is a stop-and-report (AGENTS.md's immutable-golden rule) |

**Also carried:** every other `M2_KEYS` replay in `test_engine_m2.c` (`run_period_end_probe` `:4629-4633`, `run_regulation_over_probe` `:4746-4765` — the clock/machine chains, face-independent) is expected to survive and is covered by FU323's sweep; the golden producer's transcript is H6/H7's re-pin.

---

## 8. Numbered legs

1. **FU322-L1 — the window constants are FU323's, not this doc's.** `M2_NAT_FACE_FIRST_LO/HI` are placeholders; the width rule is stated (§5 item 4) but no bound is set without FU323's measured first-event step. Setting them by eye would invent a number. Owner: FU323 + the Stage-2 P2 commit.
2. **FU322-L2 — the natural pan's seed, axis and v0 are unmeasured here.** The A4 form is arm-agnostic (v0 from the event frame); the seed tuple's identity (slot arm vs no-slot arm), the pan's axis and any literal Z pin are FU323's seed table. Owner: FU323.
3. **FU322-L3 — the claim pass's zero-delta witness moves with near-certainty and may demise.** FU319 §6's replay leaves none of the 22 settled faces at `0x100` (record 2 included), so the `:4141` face pin moves; whether the T1 gate still passes (Δ = fold(0x80 − face) ≤ 0xAB) is FU323's measured question. A demise re-shapes the pass to its measured behavior with the reason — never silently deleted. Owner: FU323 + the Stage-2 P2 commit.
4. **FU322-L4 — the offset pass's plateau tuple moves under the live face.** The `:4409-4415` tuple (lane/bound 36/36, face 0x100, Δ 0x4C) is the plateau-riding admission FU320-L1 named; FU323 measures its new form. Owner: FU323 + the Stage-2 P2 commit.
5. **FU322-L5 — the scripted witness may be pre-empted by the natural pan.** The isolated cutscene keeps the formation near the zero camera; with the live face the band record's natural pan can fire before the scripted claim, moving `first_tracked == 9` (`:3835`) and the stop/decay pins. Owner: FU323 + the Stage-2 P2 commit.
6. **FU322-L6 — the golden's first differing frame may precede the first pan frame.** The face is a multi-consumer cell (FU321-L10): a face-gate can change a rendered target/velocity before any pan moves the camera. The golden-governance ruling ("at or after the first face-driven event") is FU325's, from FU323's diff. Owner: FU323/FU325.
7. **FU322-L7 — the additivity gate's disposition is conditional.** Outcome A/B per FU323's first differing line (§6); in outcome B the stratum-set replacement must land in the same commit. Owner: FU323 + the Stage-2 P2 commit.
8. **FU322-L8 — the seam counts are AI-coupled.** `calls == 6118` is measured with the static face; the face-gated AI can move the keeper route's call count. Re-valued once, in the inversion commit, with FU323's number; `select_ran == 0` survives structurally. Owner: FU323 + the Stage-2 P2 commit.
9. **FU322-L9 — the match_frame fixtures' survive/verify status is an expectation, not a proof.** H8–H11's reasoning is code-read; FU323's measured diff is the arbiter, and any break moves in the inversion commit (the break→same-commit rule). Owner: FU323 + the Stage-2 P2 commit.
10. **FU322-L10 — the no-ISO natural window under the live face is unmeasured.** The degraded seed puts every record at the camera (lane 0) with install-rewritten zero velocity words — the zero-seed bind-only class (E25) — so the both-modes zero asserts cannot survive unconditionally; the design re-gates (§4.4) and FU323 measures the mode before any no-ISO pin lands. Owner: FU323.
11. **FU322-L11 — the follow waiver's wording ruling (this doc's, for the wave review).** The phrase "the P7/P8 window/lockout assertions" (in `:3720`, `:6049-6054`, `docs/ENGINE.md:6424`, FU321-L11) cannot mean FU313's P7/P8 verbatim on this tape: the row-04 event's height is 0, so the follow window cannot open (E16) and FU317's waiver stands. The landing pair is the **pan** window (A4) + lockout (A5); the FU313 follow window/lockout remains RED/waived for the Y/bootstrap producer (FU168-L5, out of phase). Owner: the wave review + the Stage-2 register wording.

---

## 9. GO/NO-GO

**GO — the inversion design.** The replacement set is test-side, computed from the undriven tape's own sampler; the identities (A2, A4's order, A5's off-zero) cannot re-pin on unrelated changes; the formula assert (A4) pins the arithmetic without pinning FU323's numbers; the risk class (the settled faces, the transients) is explicitly routed to records; the commit is atomic in exactly FU321-L11's sense; no native port, no staged camera/face gate, no golden-only claim. The design needs no measurement to be evaluated, only FU323's values to be filled.

**Conditional on FU323 (the fill gate):** the window constants (L1), the seed/axis table (L2), the sibling re-pin values (L3/L4/L5), the additivity outcome (L7), the seam counts (L8), the match_frame verdicts (L9), and the no-ISO measurement (L10). Any contradiction with FU319 §6's frozen prediction becomes a numbered leg with its native owner — never explained away (spec §7).

**NO-GO on:**
- (a) landing the inversion without the A4/A5 window/lockout pair in the **same** commit, or deleting/weakening the zero-camera asserts without the replacement (FU321-L11's explicit Case-B gate);
- (b) a replacement that reads the golden's hashes or the driven witnesses' cells instead of `run_natural_pass`'s own sampled cells (the plan's review-focus 3);
- (c) asserting the turn-transient/settled-face class anywhere in the natural pass (the permanent re-pin trap, §5 item 15);
- (d) hard-pinning the first event's exact step, the seed-dependent literals, or the Z=0 status without FU323's measured citation (the evidence rule);
- (e) any staging of a camera/face cell to make the natural window pan, or a period staging not declared as a match-setup input (FU321 §4.1/§6.7 — carried unchanged);
- (f) any M2 move without the frame diff + the written reason + the register pin in the same commit (FU321 §5); M1 moving at all is a stop-and-report, never an upgrade.

---

## 10. Known limits

1. **No measurement in this slice.** No probe was run; the plan's Task 1 needs none. Every post-producer number in this doc is a placeholder or a carried prediction (E17) — the binding values are FU323's.
2. **The prediction's status is open-loop.** FU319 §6's replay does not re-integrate positions/targets under the changed face (FU319 §10); the band record's conclusion is robust (its target *is* its park point), the others are not. FU323 measures.
3. **The identities' exactness assumes the sampler's order.** A2/A4's identities hold because the sampler visits every granted frame in order and the first departure is the event; a second event type inside the window (unmeasured) breaks them as an honest RED, not silently.
4. **The claim-pass reconciliation is not statically resolvable here.** FU319 §6's band census (entity 12 only) and the claim pass's gate tuple (record 2 at the camera, lane 0, `:4134-4140`) imply different record-2 walks on the two tapes — the drives' AI coupling explains it, but only FU323's diff settles what the live face does to each.
5. **This doc fixes the form, not the values.** The Stage-2 plan (P2) binds the constants from FU323 and carries the frame diff; nothing here may be cited as a measured post-producer number.
6. **The 2522-line prefix analysis is manual.** Even in outcome A the register must classify the moved frames manually (FU321-L6 stands); the stratum gate (outcome B) is a guard, not a proof.

---

## 11. Provenance and self-audit

**Fresh this session (repo reads, no writes):** `tests/test_engine_m2.c` (`:1583-1707`, `:1730-1935`, `:1977-2147`, `:2250-2370`, `:2370-2499`, `:2560-2689`, `:2793-3096`, `:3214-3300`, `:3540-3739`, `:3740-3939`, `:3940-4169`, `:4170-4399`, `:4400-4599`, `:4600-4669`, `:4746-4805`, `:6020-6153`, `:6155-6300`, `:6400-6467`); `tests/test_engine_match_frame.c` (`:140-185`, `:1103-1182`, `:2430-2549`, `:7535-7834`, `:7835-7993`); `tests/test_engine_m1.c` (`:1-70`); `src/fifa96_engine/fifa96_match_run.c` (`:480-564`, `:1002-1077`, `:3955-4004`); `src/fifa96_engine/fifa96_match_entities.c` (`:529-573`); `src/fifa96_loader/fifa96_keeper_cpu.c` (`:140-200`); `include/fifa96_engine/fifa96_match_entities.h` (`:90-170`); `docs/ENGINE.md` (`:6360-6711`); `AGENTS.md`.

**Carried (read, not re-derived):** `docs/ghidra/FU319_r2l6_direct_face.md` §2/§5/§6/§8/§10; `docs/ghidra/FU321_r2l6_nonzero_acceptance.md` §3/§4.6/§5/§6/§8/§9; `docs/ghidra/FU313_r2l5_natural_acceptance.md` §5.3/§9; `docs/ghidra/FU317_r2l6_claim_acceptance.md` (the P6/P7/P8 mapping and the follow waiver); `docs/superpowers/specs/2026-10-22-fifa96-m2-phase33-live-record-face-design.md` §2/§3/§6/§7/§8/§9; `docs/superpowers/plans/2026-10-22-fifa96-m2-phase33-recon-ahead.md` (Task 1 + the global constraints + the review focus).

**Compliance:** the only repo file created is this draft; no git commands; no build-dir writes; no probe; `game/FIFAPCCD96.iso` and `tests/golden/` never written; `/tmp/opencode/fu322/` unused (nothing needed a scratch binary). No prior doc section is rewritten: the binding docs' `:3701-3707`/`:4007-4263` cites stand as written and are quoted here with the fresh numbers per the E19 errata (the ENGINE.md errata already record the drift; this doc re-states it, it does not correct anything).

**Consistency checks run:** every file:line in §3–§7 was read this session (no cite is inherited from another doc's line numbers except where marked *carried*); the three asserts' exact text was read at `:3722-3724`; the m-42/m-58 staging and the additivity gate were read at their fresh sites; the follow gates were read in the port source. The `T(s)` helpers are specified against the already-pinned arithmetic (E9/E10), not re-derived from native bytes.

---

## Review erratum (wave review, 2026-10-22)

**1. The premise "the undriven natural tape pans" is falsified by the wave's own measurement.** Quotes (§1, the question): "With the live `+0x43` record face staged (FU319 §5), the undriven natural tape pans — what exactly does the inversion commit do to …"; (§1, the answer): "are replaced — never deleted — by an inverted set that pins the **one-shot pan window** the live face opens on the undriven `M2_KEYS` tape"; (A1's comment block): "with the live +0x43 latch the / \* undriven tape pans from the first face-driven event". **Measured (FU323 §2/§4, closing FU319-L7; leg FU323-L1):** the undriven patched tape does **not** pan — the band record (entity 12) holds no control slot (`has_slot 0` in the probe dump at the band frame, step 5062), so the row-04 seed switch takes the **no-slot arm** (seed = `word[+0x73/+0x75] << 2` = ±4 for a parked record's stride velocity ±1) → `v0 = trunc(±4/0xC) = 0` → a **silent code-5 bind** at granted f=1517 (tick 5060: `trk=12`, `tim=0/12`, `ramp=12`, `vel=(0,0)`) with `pos` staying `(0,0,0)` through f=3597; the aggregate cells are `camera_zero_frames` **1517** / `camera_left_zero` **2081**. The honest post-producer baseline is: **faces live, the silent code-5 bind, camera still zero undriven**. Re-shaped ruling per assert: (A1) the assert `camera_left_zero >= 1` **passes measured** (2 081 ≥ 1) but its comment is void — the departure it pins is the **bind**, not a pan; the comment must be re-worded to the measured consequence ("the row-04 gate passes once and binds a record with a zero seed — the FU323-L1 silent-bind baseline") and A1 still means a real live-face consequence only together with the bind identity, not with a pan window. (A2) the identity `camera_zero_frames == first_left_zero_f` holds measured (1517 == 1517 — the pre-bind prefix) and stands as written. (A3) the window constants anchor to FU323's measured **bind** (granted f=1517 / engine step 5065), not a pan event; the bullet's "the row-04 gate's phase term makes any face-driven event strictly after the phase-2 entry" is measured-safe for this window but is not pan-law structure — the code-3 walk camera-event channel is the counterexample class the patched drives measure.

**2. A4/A5 are pan-window asserts; they are void on the undriven tape and belong to the DRIVEN witnesses.** Quote (§4.1 A4/A5; §5 item 6): "`assert(nat->pan.segments == 1u);` …" / "| 6 | `segments == 1` | **LOCKED (from FU323)** | the one-shot; …". **Measured:** no granted frame has `pos ≠ (0,0,0)` in the window, so the delta-run machine records `segments == 0` / `changed_frames == 0` undriven — A4's `segments == 1`, `first_motion_f == first_bind_f + 1` and the `T(s)` formula asserts, and A5's `off_zero_frames` identity, would RED on the very baseline they were designed to pin. The measured pans exist only on the **driven** tapes, where FU323 §5.1/§5.2 rules both phase-32 witnesses RE-PIN with real pans: claim pass gate f=1552 (code **3**, lane_x 128 / lane_z −128, bound 128, `has_slot 0`, face7d 0x300, bearing/Δ 0/0), event f=1553 (code 3, trk 1), segments **2**, first_motion 1554, stop 1581, 54 changed / 2 116 off-zero frames, place `(−0x120,0,0)`; offset pass gate f=1552 (lane_x **129** = bound 129, cam_dz6f −4, Δ=0), segments **2**, stop `(−288,0,0)`. The window/lockout pair (the P7/P8 landing named by FU322-L11) is therefore **relocated to the claim/offset passes** and shaped from those measured cells — two chained segments (the kept trace shows the first segment's row-04-coda shape: bind f=1483, `trk=2` window f=1483..1552, −0x90 pan, stop f=1511; the second is the new code-3 walk camera event the frozen world did not have), the per-segment `T(s)` arithmetic asserted from the recorded seeds, `row04_after_event_frames == 0` — never as one-segment/one-literal undriven asserts. The bounds items stand: `stop_y == 0` / Y == 0 (measured: the undriven `pos` prefix and the driven `y_nonzero_frames == 0`), `goal_armed_any == 0` / `oob_frames == 0` (measured 0 on both driven passes) — but they are **not in FU323's undriven dump**, so undriven they stay recorded-only until measured; §5 item 11's `post_event_row04_frames == 0` is likewise measured only on the driven passes (FU323 §5.1) and demotes to recorded-only undriven.

**3. The FU319-L3 pacing premise is void (FU324's falsification).** Quotes (A3's derivation bullet): "the event frame is face-coupled pacing (the turn transient + the band entry), which the separately-pinned `move_attr` metric sub-change (FU319-L3) moves"; (§5 item 4): "the window's width covers the declared pacing slack (≥ the `move_attr` sub-change's transient shift, FU319-L3)". **Measured (FU324 §2/§5, leg FU324-L1):** the metric sub-change is a **measured no-op** — the two native metric functions are instruction-level the same computation, the verbatim C ports agree on 4 000 800 pairs, and the metric-only patch moves **0** frames alone and **0** frames composed on the FU323 latch build; FU319-L3's "2 straddles at 4912/5292, 97 vs 96, max |Δ| = 115" does not reproduce. No metric transient shift exists to cover; the LO/HI slack rationale must be re-based on measured pacing variance, and the "stride index moves for every record" fear (FU319 §7-L3's separate-pin rationale) is measured false in the window (the `+0x7B` nibble ≤ 3 clamp masks any metric difference; it returns only if a nibble staging lands — FU324-L5).

**4. The golden run's governance anchor is measured, not the m-42 staged dispatch.** Quote (§6): "In the golden run the 'first face-driven event' is the m-42 staged dispatch's face-gated pass (whose face operand is the staged producer input, E12)". **Measured (FU323 §6/§7):** the golden run's first face-driven event is the tape's own chain — the faces settle live at engine step 9, the first face-driven position movement is frame 52 (t0#6, the mover's stride-error face term, `action_handlers.c:257-258`), the first hash-visible frame is **852** (the sit-1 half-time kickoff's camera reset revealing the since-372–379 face-driven t1 walk-in sprites); the m-42 staged dispatch is directive-staged and sits inside the moved window. The spec §6 rule ("the first differing frame at or after the first face-driven event") holds measured (852 ≥ 9/52); the governance ruling is FU325's and FU322-L6's question is answered.

**5. Verdict update and the mode-gating disposition.** Quote (§9): "**GO — the inversion design.** … The design needs no measurement to be evaluated, only FU323's values to be filled." **Measured:** the undriven A4/A5 form is not fillable (there is no motion to pin); the GO stands only for the **corrected set** — the bind-anchored A1–A3, the window/lockout pair relocated to the driven witnesses per FU323 §5, and the chaos class routed to records per §5 items 10/15/16 — which the Stage-2 P2 commit implements from FU323's §4/§5 tables as the written reason. The mode gating (§4.4: the ISO-gated inverted set; the degraded no-ISO window recorded-only) **stands unchanged** (leg FU322-L10). The §6 additivity disposition is decided by measurement: FU323's first differing golden line is frame 852 — **inside** the v21 2 522-line prefix — so **Outcome B** applies and the stratum-set gate lands in the same commit; FU323 measured the stratum set unchanged (the moved lines are the pre-existing `state=1` ×951, `state=19` ×2 620, `state=2` ×2 100 — 951+2620+2100 = 5 671). New leg: **FU322-L12 — the re-shape dependency:** the inverted baseline is re-shaped around the measured silent bind and the driven pans; owner: the Stage-2 P2 commit, binding only numbers FU323's tables measure. The compliance record (§11–§12) is unaffected: no probe, no git, the ISO untouched.
