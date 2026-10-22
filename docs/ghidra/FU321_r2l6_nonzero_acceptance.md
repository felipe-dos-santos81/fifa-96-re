# FU-321 — the non-zero-delta acceptance and the M2 golden risk (phase-32 R2-L6)

**Phase/Provenance:** M2 phase-32 recon-ahead wave (Stage-1, **task 4 of 4 — `FU321: the acceptance and the golden risk`**), 2026-10-22, read-only. Deliverable of `docs/superpowers/plans/2026-10-22-fifa96-m2-phase32-recon-ahead.md:89-103` under `docs/superpowers/specs/2026-10-22-fifa96-m2-phase32-face-aligned-pan-design.md` §2/§3.4/§5–§9. **No fresh Ghidra read in this slice:** every native fact below is *carried* from FU314/FU316/FU317 (each row names the frozen doc/section or the port line that transcribes the native bytes); the program `/FIFA96.EXE` was not consulted, because the slice's claims are (a) the port's pan arithmetic, (b) the port's acceptance harness, (c) the test/golden mechanism — all first-hand in the C sources read this session (`file:line` cited). The M1/M2 pins are a fresh `sha256sum` this session (§5.5). No probe was run (none is needed: no new measurement is claimed). No repo file was created or changed except this draft; no git; the ISO untouched.

---

## 1. The question, and the one-page answer

> **Given a non-zero-delta pan (the offset gate tuple), what exactly will the acceptance show, what are the expected trace cells and place delta, how is the M2 re-pin governed if a producer slice (FU314 L5/L3) moves the sim, and which hazards must the Stage-2 plan carry?**

**The answer in one paragraph.** The gate offset does **not** enter the pan's seed. The seed is the row-04 arm-B pair chosen by `camY / has_slot` (`fifa96_outfield.c:685-701`), and the pan is the affine `T(s)` of FU316 §4.3 (`camera.c:770-830` + `camera.c:959-991`). So a non-zero-delta pan is expected to move the place by **the same `T(s)` family as the measured zero-delta `−0x90`**: with the gate record holding a slot and the same dir, exactly `(−0x90, 0, 0)` over 27 frames, `pan 0→4`; per axis the reachable range is **`|Δ| ∈ [0, 0x90]`** (0…144) with **0…27** changed frames, the value being read off the seed table (§3.6). Y stays `0` universally (height `0` ⇒ the idle ramp; `camera.c:92-93/716`), and Z is `T(s_z)` — zero **only if** the seed's z is zero (the phase-31 `Y=Z=0` is that drive's property, not a pan law). The **new** assertion the acceptance must add is the **gate offset itself** (`lane_at_pass ≥ 1` ⇔ `(cam_dx, cam_dz) ≠ (0,0)`), because the trace carries camera cells only (`match_run.c:4591-4607`) and **cannot** prove the offset. The Layer-2 procedure is the frozen T3 one (`docs/screens/p31-l2-notes.md:10-17/43-61`, FU317 §5) with FU320's drive substituted and a phase-2-relative start; the M2 policy is the spec's **faithful-move** (§7) with a concrete frame-diff format, the exact `cmp`/sha commands and the fixture hazards (§5). **GO on the design; the execution is conditional on FU320 returning a drive with `lane_at_pass ≥ 1` and a seed with `v0 ≥ 1`.**

Corollary worth stating up front, because it is the trap this doc exists to prevent: a "non-zero-delta" witness can **re-prove the zero-delta case** either by passing at `lane = 0` (the bearing folds to `0x80`, FU317-L10) or by passing with a **zero seed** (`word[+0x73/+0x75]` = 0 ⇒ `v0 = trunc(0/0xC) = 0` ⇒ the event binds `trk` but the place never moves). Both are NO-GO; §4.6 (G3/G4) and §6.8 give the exact fail signals.

---

## 2. The evidence table

| # | Claim | Evidence (fresh file:line, or the formula derived from it; native facts *carried*) |
|---|---|---|
| E1 | The row-04 half-line arms pass **height `0`** to the camera event; arm B (`0x1D`) passes `event_track_reload = 0`, arm A (`0x15`) `1` | `handlers.c:2785-2787` (`fifa96_camera_event_set(…, out.event_x, out.event_z, 0, out.event_track_reload)`); native `0x7EFC4/0x7F0C6 XOR EBX,EBX` (carried FU314 §2.1) |
| E2 | Height `0` selects the **idle ramp**: `F2 = 6, F4 = 0xC = timer_limit, F6 = 0, F8 = 0xC = ramp_divisor`, `event_param = 0` | `camera.c:753-762`; carried FU316 §4.2 (its `decompile_function 0x70544`) |
| E3 | The seed switch has exactly three arms: `camY != 0` → `dir[type]<<5`; `camY == 0` + slot → `slot_dir<<6 = ±0x40`; `camY == 0`, no slot → `word[+0x73]<<2, word[+0x75]<<2`, else `(0,0)` if `|cam_x| > 0x630` or `|cam_z| > 0xA20` | `outfield.c:685-701` (`:687-688`, `:690-691`, `:697-698`); native `0x7F003/0x7F035/0x7F056` (carried FU314 §2.2, FU316 §4.3) |
| E4 | The gate record's `has_slot` is the live pool `+0x20` cell, not "the human record" | `handlers.c:2627` (`s.has_slot = r->has_slot`), `match_run.c:1327` (`r->has_slot = e->has_slot`), `entities.c:278/315-316` (the slot-pool writer); carried FU315 §4.3 |
| E5 | The gate record's dirs are the **slot** dirs when it holds the slot, else its own pool dirs | `match_run.c:1406-1412` (`mr->slot.entity == id → anim_b/anim_c`, else `e->dir_x/dir_z`), staged at `handlers.c:2655-2656` |
| E6 | The fast path divides the seed by `F8 = 0xC` once (`pan_counter == 0`), then each pan step is the slow path `k = 24`, `/ 0x20` | `camera.c:770-777`; rates `camera.c:942-944` (`pan_decay 10`, `pan_rate_hi 16`, `pan_rate_lo = 16 + (32−16)/2 = 24`); native `0x15781B` (carried FU316 §4.2) |
| E7 | The pan step runs when `timer > timer_limit (0xC)`; the integrator is `pos += vel * delta`; the stop is `speed == 0` | `camera.c:97-100` (the step), `:101-111` (integration), `:148` (speed recompute), `:86` (the outer gate); carried FU316 §4.3 |
| E8 | `delta = 2` on every granted frame (100 Hz tick, `0x200` step, `whole = acc>>8 = 2`; 60 units/s ⇒ 30 grants/s) | `match_run.c:4411-4417` (`(int16_t)mr->state.frame_delta`), `match_state.c:47-56`, `fifa96_match_state.h:6-7` (`STEP 0x200`, `SECOND 0x3C`) |
| E9 | `v0 = trunc(s / 0xC)`, `v_{i+1} = trunc(24·v_i / 32) = trunc(3·v_i/4)` (C truncation toward zero), `T = 2·Σ n_i v_i` with `n_0 = 6`, `n_i = 7`, frames `6 + 7(n−1)` | derived from E6–E8; the 5-point check in §3.4 reproduces FU316 §4.3's `T(0x40)=0x90, T(0x60)=0x140, T(0x80)=0x174, T(0xA0)=0x1FA, T(0x100)=0x3B8` exactly |
| E10 | The measured zero-delta instance: seed `−0x40 → vel −5, speed 5, 27 frames, place −0x90, pan 0→4` | `docs/ENGINE.md:5896-5919` (T1 pins), `p31-l2-notes.md:19-33`, FU317 §3.3 |
| E11 | The gate angle is `fold(bearing − face(+0x7D))`, `bearing = 0x8DD70(cam_dx, cam_dz)`, pass `≤ 0xAB` | `outfield.c:629-635`; `match_run.c:1499-1507` (`cam_dx = cam_x − pos_x`); carried native `0x7EE4B..0x7EE7D` (FU314 §2.2) |
| E12 | `fifa96_entity_angle(0, 0) = 0x80` — the zero-offset fold; the angle convention is `0=+z, 0x100=+x, 0x200=−z, 0x300=−x` | `entity_update.c:90-92`; the octant map carried FU314 §9 (`angle_table.c`) |
| E13 | The trace carries **camera cells only**: `f clock ph pos(x,y,z) vel(x,z) spd fspd ev cur trk pan tim ramp rate follow tick`; `ev = event_param` (the height), not the event code | `match_run.c:4591-4607`; the header `match_run.c:1849-1850` |
| E14 | The frame order is camera walk (`:4413`) → entity chain (`:4471`), so an event set in frame *f* moves the place in *f+1* | `match_run.c:4411-4417` then `:4447-4473`; carried FU317 §3.1 |
| E15 | The delta-run measurement is the maximal changed-`pos` run; the off-zero count is never the count (measured `2 084` vs the 27-frame run) | `docs/ENGINE.md:5908-5910`, FU317 §3.4 |
| E16 | The one-shot is the row-05 install (`0x7F133`); the tracked exclusion is the latent second guard; a row-6 diversion exists | `outfield.c:719` (self, code 5), `:682` (the exclusion), `:709-716` (the row-6 arms); carried `docs/ENGINE.md:6025-6037` |
| E17 | The port's `is_ball_track` falls back to `id == controlled` when `tracked == NONE`, blocking the controlled record's arm B | `handlers.c:2675-2681`; carried `docs/ENGINE.md:6038-6049`, FU314 §3, FU316-L3 |
| E18 | The M1/M2 pins: M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (22 004 B), M2 `8434cfab105b926e78279ca3ea03c62bd3a0ae9654d2e11f0930c0cb8605cee1` (295 087 B) | fresh `sha256sum` + `wc -c` this session (§11) |
| E19 | The golden comparison is an in-test `memcmp` with the first differing offset reported; the regeneration is a stdout redirect; the additivity gate asserts the v21 2 522-line prefix | `tests/test_engine_m2.c:6049-6092`, `:6063-6082`, `:1741` (`#define M2_GOLDEN_PATH`), `:31` (the documented command) |
| E20 | The phase-30 re-pin precedent (the frame-diff wording and the sha ledger) | `tests/test_engine_m2.c:1697-1707` (quoted §5.2) |
| E21 | M2 already carries a **non-zero-delta, face-aligned, slot-seeded** row-04 pan in the m-42 band / link fixture: camera park `(0,0,0xB00)`, record `(0,0,0xAF0)`, `lane 0x10`, `face 0`, slot dir `(0,+1)` → `vel_z +5`, the pan crosses the armer at `0xB28` | `tests/test_engine_m2.c:2825-2836` (the park), `:2845-2870` (the staged record incl. `face7d = 0`, `vel73/75 = 0`), `:2887-2893` (the pan asserts) |
| E22 | The m-42 fixture's staged `face7d = 0` equals the pre-fix constant ⇒ that fixture cannot detect a re-staging regression | the site `tests/test_engine_m2.c:2862`; carried `docs/ENGINE.md:6150-6152` |
| E23 | The layer-2 capture facts: `960x591+0+0` crop, AE_pan `73 402` > AE_still `27 849`, HUD-crop AE `0`, bbox `317×382` (sprite-only, Gate 2 fails) | `p31-l2-notes.md:13/37-61`, `docs/ENGINE.md:5962-5974` |
| E24 | The armer/OOB bounds: OOB `|x| > 0x6C0 ∨ |z| > 0xAB0`; armer `|z| > 0xB20` first, else `|x| > 0x730` | `camera.c:189-191`; carried FU316 §5.1 |
| E25 | The natural-pass baseline is a **test-side** assert (not only a golden): `camera_left_zero == 0`, `camera_zero_frames == granted_frames`, `first_left_zero_step == 0`, with the site's note "the bootstrap producer will flip it and the P7/P8 window/lockout assertions land with it (FU313-L1)" | `tests/test_engine_m2.c:3697-3707` |

---

## 3. The pan arithmetic (the formula, the range, Y/Z)

### 3.1 What the gate tuple actually decides

The gate tuple of FU314 §6 is: phase 2, `code == 4`, `active`, `timer81 == 0`, `lane ≤ 0x40 − timer81`, `lane ≤ bound`, `camY ≤ 0x50`, `|fold(bearing − face)| ≤ 0xAB`, `row_byte != 0x13`, not tracked. Of those terms, **only one selects the pan's seed**: the `camY`/`has_slot` switch (E3). The face/bearing term selects *whether the pan fires at all*; it never appears in `T(s)`. Therefore:

> **The pan's place delta is seed-determined, not offset-determined.** A gate pass at a real bearing and a gate pass at `pos == cam` with the same seed produce the **same** place delta.

That is the direct answer to the plan's step 1: the non-zero-delta pan's expected delta is the measured `−0x90` class, not a new number. What *is* new is only the gate frame's geometry (E11/E12).

### 3.2 The seed at a non-zero-delta gate

At the gate the camera is parked at `(0,0,0)` (Y-waived) ⇒ `camY = 0`, because the row-04 staging reads `s.ball_height = camera.pos_y` (`handlers.c:2707`) and the parked camera's Y is 0 (E10). So only two arms are reachable:

- **slot arm** (`has_slot != 0`, the driven/measured case, E4/E5): seed `= (dir_x<<6, dir_z<<6) ∈ {(±0x40,0), (0,±0x40), (±0x40,±0x40)}`. The phase-31 witness was `dir = (−1,0) → s = −0x40` (E10).
- **no-slot arm** (`has_slot == 0`, the AI/other-record case): seed `= (w73<<2, w75<<2)`, `w = word[+0x73/+0x75]` = the record's locomotion velocity word (E3). The `|cam| ≤ 0x630/0xA20` sub-gate is always open from a `(0,0,0)` park.

The witness's arm is decided by the gate record's `has_slot`, i.e. by *which record* FU320's drive brings into the band — not by the offset.

### 3.3 The ramp and the cadence

Fast path (the event frame): `v = trunc(s/0xC)` per axis (E6; C truncation toward zero, so `−0x40/0xC = −5`, matching the measured `vel = −5`), `timer_limit = 0xC`, `timer = 0`, `ramp_divisor = 0xC`. Then, per granted frame (`delta = 2`, E8):

1. `timer += 2`; while `timer ≤ 12` the integrator uses `v` (six frames);
2. at `timer = 14` the pan step runs: `pan_counter++`, `v ← trunc(3·v/4)`, `timer = 0`;
3. the same frame integrates with the new `v` (seven frames per velocity);
4. when `speed = 0` (both axes zero) the pan step zeroes the velocity and the integration is skipped — the **stop frame** (measured `f=1541`, `vel=0, spd=0, pan=4`, E10).

So one axis is `6` frames at `v0` then `7` frames per decayed value, `frames = 6 + 7(n−1)`, `T = 2·(6·v0 + 7·v1 + … )`. Both axes decay independently with the same cadence; the shared stop is `speed == 0` (E7). No random walk: the pan-step walk gate is `(input_range[0]|input_range[1]) & 4`, image-default `0` (`match_run.c:4406-4416`), so `camera.c:976-988` is inert.

### 3.4 `T(s)` — the closed form, checked against five published instances

`T(s) = 2 · Σ_{i=0}^{n-1} (6 if i == 0 else 7) · v_i`, `v_0 = trunc(s/0xC)`, `v_{i+1} = trunc(3 v_i / 4)`, `n` = the number of nonzero `v_i`.

| `s` | `v` chain | `n` | frames | `T` | published (FU316 §4.3) |
|---|---|---|---|---|---|
| `0x10` | 1 | 1 | 6 | 12 = **0xC** | — |
| `0x20` | 2, 1 | 2 | 13 | 38 = **0x26** | — |
| `0x40` | 5, 3, 2, 1 | 4 | 27 | 144 = **0x90** | **0x90** (measured, E10) |
| `0x60` | 8, 6, 4, 3, 2, 1 | 6 | 41 | 320 = **0x140** | **0x140** |
| `0x80` | 10, 7, 5, 3, 2, 1 | 6 | 41 | 372 = **0x174** | **0x174** |
| `0xA0` | 13, 9, 6, 4, 3, 2, 1 | 7 | 48 | 506 = **0x1FA** | **0x1FA** |
| `0x100` | 21, 15, 11, 8, 6, 4, 3, 2, 1 | 9 | 62 | 952 = **0x3B8** | **0x3B8** |

The four published large-seed values and the measured `0x90` reproduce **exactly** from the code path (E6–E9). That is the strongest available validation short of a run, and it is why the table can be used as an acceptance oracle.

### 3.5 The no-slot arm's reachable values

The gate record's velocity word `w = word[+0x73/+0x75]` is the mover's stride velocity. The stride flat `0x10F680` (0x500 words) has `max |value| = 15` (fresh parse of `src/fifa96_engine/fifa96_match_locomotion_tables.c`, §11); the mover ramps toward the table target by halves (`action_handlers.c:263-279`). For `|w| ≤ 15`:

| `|w|` | `s = 4w` | `v0` | chain | frames | `T` | `pan` end |
|---|---|---|---|---|---|---|
| 0,1,2 | 0,4,8 | 0 | — | **0** | **0** | 0 |
| 3,4,5 | 12,16,20 | 1 | 1 | 6 | **0xC** | 1 |
| 6,7,8 | 24,28,32 | 2 | 2,1 | 13 | **0x26** | 2 |
| 9,10,11 | 36,40,44 | 3 | 3,2,1 | 20 | **0x4E** | 3 |
| 12,13,14 | 48,52,56 | 4 | 4,3,2,1 | 27 | **0x84** | 4 |
| 15 | 60 | 5 | 5,3,2,1 | 27 | **0x90** | 4 |

Two consequences: (i) the no-slot arm's maximum is the **same `0x90`** as the measured slot pan; (ii) `|w| ≤ 2` is a **zero-seed event** — the place never moves (§6.8). Arm-written transients (`vel73 = dir<<shift` from the jump/0x0C rows, not in the stride domain) could seed larger; none is a row-04 code-4 steady state, so this is a leg (§8, FU321-L3).

### 3.6 The expected place delta for the non-zero-delta witness — the range

| case | seed | expected `Δx` | expected `Δz` | frames | `pan` end |
|---|---|---|---|---|---|
| slot arm, `dir = (−1,0)` (the phase-31 drive axis) | `−0x40` | **`−0x90`** | `0` | 27 | 4 |
| slot arm, `dir = (+1,0)` / `(0,−1)` / `(0,+1)` | `±0x40` | `±0x90` / `0` / `0` | `0` / `−0x90` / `+0x90` | 27 | 4 |
| slot arm, diagonal `dir` | `(±0x40,±0x40)` | `±0x90` | `±0x90` | 27 | 4 |
| no-slot arm | `(4·w73, 4·w75)` | `T(4·w73)·sgn` | `T(4·w75)·sgn` | 0/6/13/20/27 | 0–4 |

**Stated as a range, not a guess: per axis `|Δ| ∈ [0, 0x90]` (0…144), changed frames `∈ {0, 6, 13, 20, 27}`; the modal expectation for a driven record that holds a slot is `|Δ| = 0x90` over 27 frames — i.e. the measured `−0x90` itself — and the acceptance must read the observed pair `(|Δ|, frames)` off the seed actually recorded at the gate (E5/E3), never assume `0x90`.**

The direction is the **seed's** sign (the record's slot/velocity dirs), not the bearing: a west-side record facing east pans by its own dir, which may be any of the eight directions. That is why the place delta cannot be predicted from the gate geometry.

### 3.7 Y and Z expectations

- **Y: `0` on every line, universally.** Height `0` ⇒ `event_param = 0` (E1/E2); `camera_update_walk` writes `pos_y` only when `event_param != 0` (`camera.c:92-93`), and `fifa96_camera_height` returns `0` when `event_param ≤ 0` (`camera.c:716`). The Y producers are the phase-30 circular route (waived); no row-04 pan can move Y.
- **Z: `T(s_z)` — zero iff the seed's z component is zero.** The phase-31 `Y=Z=0` (`docs/ENGINE.md:5911`, `p31-l2-notes.md:29`) is a property of the DOWN/dir-`(−1,0)` drive, **not** a pan invariant. The new note must state the observed axis and seed; "Z = 0" may only be written when the seed's z word is 0.
- **OOB/armer: no single pan from `(0,0,0)` can reach either bound** (`|Δ| ≤ 0x90 < 0x6C0 < 0x730`, `0x90 < 0xAB0 < 0xB20`; E24) — including the diagonal case (`0x90 < 0x730` and `< 0xB20`). So `goal_armed = 0` is a genuine expectation, not a hope.

### 3.8 The offset criterion (what "non-zero-delta" must mean in the pins)

```
bearing = angle(cam_dx, cam_dz)            # +0x6D/+0x6F, cam − pos   (E11)
Δ       = fold(bearing − face(+0x7D))      # & 0x3FF, to 0..0x200      (outfield.c:632-635)
pass    ⇔ Δ ≤ 0xAB
```

`fifa96_entity_angle(0,0)` returns `0x80` (E12), so **the zero-delta degeneracy is exactly `(cam_dx, cam_dz) == (0,0)`, i.e. `lane == 0`.** The binding Layer-1 pin for this slice is therefore:

> at the **passing dispatch**, `lane_at_pass ≥ 1` (equivalently `pos ≠ cam`, `(cam_dx,cam_dz) ≠ (0,0)`), with `Δ ≤ 0xAB` evaluated on the **live** face (no staged `face7d`, no staged camera/gate cell).

That is a *different* pin from the phase-31 one (`lane == 0`, the fold) and it is the only new Layer-1 assertion FU321 adds. Note Δ = `0x80` is **not** the degeneracy signature: a record due west of the camera with `face 0x100` passes at Δ = 0. The signature is `lane == 0`.

For the static-face route (FU314 R2) the pass set with `face = 0x100` (+x) and the camera at the origin is the **west cone** `pos_x < 0` with `|pos_z| ≲ |pos_x|·tan 60°`; for a live face it is the ±60° cone around the face direction (E11). Both must still satisfy `lane ≤ 0x40` and the approach term `lane ≤ bound` (the previous frame's lane) — i.e. the pass frame is on a **decreasing** approach, which is the second timing-sensitive term (§6.4).

---

## 4. The Layer-2 acceptance (the design)

### 4.1 Prerequisites and interface

FU321 consumes, and does not re-derive, **FU320's tuple**: the drive (keys + tape indices/tick offsets), the record id, the gate frame, and at the gate the cells `pos`, `lane`, `bound`, `has_slot`, `dir_x/dir_z`, `word[+0x73/+0x75]`, `face7d`, `(cam_dx,cam_dz)`, `tracked`, `controlled`. From those cells §3.2/§3.6 yield `s` and `T(s)`, i.e. the **expected trace cells**. If FU320 returns NO-GO, the acceptance lands as this procedure + the arithmetic + the M2 policy and the witness becomes a leg (§9).

Hard preconditions (binding, from the spec §5/§8/§9): no staged camera/gate cell, no staged `face7d`; the fixture (if any) stages only producer inputs; the period may be staged **only** if FU320 proves R2 needs it, declared as a match-setup input (spec §9.3).

### 4.2 The drive and its timing

The preamble is the frozen T3 tape-equivalent (the bare hold fails — `p31-l2-notes.md:72-80`): intro RETURN → front-end BACKSPACE → RETURN (match); `M2_KEYS` prefix replayed by trace `tick` — RIGHT 7…47 (KICK press/release 14/17), LEFT 47…140, the act-1 KICK press/release 1790/1797; phase 2 lands at **tick 1897 (`f=568`)**, the walk/claim timing is **absolute** (gate/event/motion at ticks **5044/5047/5050** = `f=1512/1513/1514`, `docs/ENGINE.md:5948-5957`).

FU320's drive replaces only the **post-phase-2** segment. Start it as a **phase-2-relative offset** (T3 used phase 2 + 20 ticks = 1917), never as an absolute tick, because the live phase-2 landing drifts (+95 ticks vs the null tape). Release after the trace shows the stop, mirroring T3's keyup at tick 5270 (`f=1580`) so the still pair is free. Record **both** coordinates of every anchor: the null-tape index (Layer-1) and the live tick/`f` (Layer-2); the acceptance note carries the pair and the delta. This is FU321-L4 (the new drive's live timing is unmeasured).

### 4.3 The trace cells (what the acceptance asserts, line by line)

The trace is the attribution artefact (E13/E14). Run `DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p32-cam-trace.log make game` (`Makefile:39-41`; the sink is default-off and never touches stdout/goldens, `match_run.c:1828-1855`).

| trace line | expected cells | why |
|---|---|---|
| the gate frame (the line before the event) | `pos` = the pre-pan place (the parked `0,0,0`), `vel=0,0`, `spd=0`, `trk=-1`, `pan=0`, `ev=0x0`, `tim=0/0`, `ramp=0`, `follow=0` | the camera is still parked; the row-04 gate frame (Layer-1's offset tuple is *not* in the trace) |
| the **event** frame | `pos` unchanged; `vel=(v0x,v0z)`; `spd=|v0|`; `trk=<FU320's record id>`; `pan=0`; `ev=0x0`; `tim=0/12`; `ramp=12` | `fifa96_camera_event_set` idle arm (E1/E2) + `fifa96_camera_set_tracked` (`handlers.c:2796`); the height-0 arm ⇒ `ev=0x0` even though the event fired |
| the **motion** frames | `pos` strictly monotone along the seed's sign; `vel` decaying `v0→…→1`; `pan` incrementing; `trk` unchanged; `pos_y=0` | `camera.c:97-111/770-777` |
| the **stop** frame | `vel=0,0`, `spd=0`, `pan = n` (4 for a `v0=5` pan), `pos` = the final `T(s)`, `trk` unchanged | the `speed == 0` stop (E7) |
| the whole run | `follow=0` and `pos_y=0` on **every** line; one header + the body lines; contiguous `f=`; `ph` on every line | the Y waiver; the FU313 §4.3 trace contract |

Two reading traps to state in the note:

1. `ev` is `event_param` (the **height**), not the event code (E13). The pan event is invisible in `ev`; the event's signature is `trk` changing to the record id + `vel` jumping to `v0`.
2. `pan` counts pan *steps*, not frames: a `v0 = 1` pan ends at `pan = 1` over 6 frames; `v0 = 5` ends at `pan = 4` over 27 (E9). A `pan = 0` run with `trk` set is the zero-seed fail (§6.8).

### 4.4 The same-clock pairs, the crop, the AE, the bbox

The procedure is FU317 §5.3 / `p31-l2-notes.md:37-61`, unchanged except the pair window:

```
DISPLAY=:1 ffmpeg -f x11grab -framerate 60 -video_size 960x720 -i :1+66,69 -t 70 /tmp/opencode/p32-live.mkv
# extract two frames inside ONE displayed second AND inside the trace's delta run
for p in pan-a pan-b still-a still-b; do convert /tmp/opencode/p32-$p.png -crop 960x591+0+0 +repage /tmp/opencode/p32-$p-c.png; done
compare -metric AE /tmp/opencode/p32-pan-a-c.png /tmp/opencode/p32-pan-b-c.png null: 2>&1
compare -metric AE /tmp/opencode/p32-still-a-c.png /tmp/opencode/p32-still-b-c.png null: 2>&1
compare -compose src /tmp/opencode/p32-pan-a-c.png /tmp/opencode/p32-pan-b-c.png /tmp/opencode/p32-pan-diff.png
convert /tmp/opencode/p32-pan-diff.png -format '%@' info:      # the bbox
```

- **Gate 1 (relative, binding): `AE_pan > AE_still`**, both measured in the same session/build, both recorded with the pair's displayed second and the trace interval (E23; no absolute floor — FU309-L6).
- **Same-clock rule:** both pan frames carry the **same displayed `MM:SS`** and both lie inside the delta run with `trk` = the record. A 27-frame pan is ~0.9 s (30 grants/s, E8) — the 0.25 s pair of T3 fits; the minimum **6-frame** pan is 0.2 s (~12 clip frames at 60 fps), so the pair must be ~0.05–0.1 s apart and the clip fallback of `p31-l2-notes.md:82-84` is mandatory.
- **Still control:** same run, no key held, a second whose `pos` is constant (T3: 2 s after the pan).
- **HUD crop:** `AE = 0` within each pair on the clock crop (the pixels must be identical; T3's `140x50+0+0`/`140x66` pair).
- **Gate 2 (whole-scene bbox) is expected to FAIL and is never an attribution**: the renderer draws sprites on black, so the diff is sprite-limited (`317×382` vs the proposed `864×443`; E23). Record the bbox, label the caveat, claim nothing from it. The visible shift itself scales with the seed: `≈ 50 px/320 ≈ 150 px/960` at `Δ = 0x90` (FU317 §3.7) and proportionally less for the no-slot values — so the AE margin shrinks with the seed (FU321-L7).

### 4.5 The attribution rules (binding)

1. **The trace attributes; the AE corroborates.** The trace is the only artefact that shows *when* and *which record*; the AE shows only that *something* changed (E23; FU309-L5 closed, the trace-primary rule; FU309-L6, the relative gate with the same-session still control).
2. **The trace and the captures are the same run** (the same `FIFA96_CAMERA_TRACE` file, the same clip).
3. **Same-clock pairs only; both frames inside the delta run.**
4. **The still control is re-measured in-session**; no absolute floor.
5. **A band-limited bbox is entity motion, not a pan.**
6. **The offset is a Layer-1 claim.** The live trace has no record `pos`/`lane`/`face` cells (E13): Layer-2 reproduces the *drive* and the *pan arithmetic*; the non-zero-delta tuple is cited from the Layer-1 run of the same drive. The note must say exactly that — never "the trace shows the offset". (FU321-L1.)
7. **No golden re-pin for a capture.** A live path that moves M1/M2 stops the task for the written diff (§5).

### 4.6 The acceptance matrix

| gate | assertion | fail signal |
|---|---|---|
| **G1 trace** | exactly one delta run in the drive's pan window; `trk` = the record id at the event and after; `pan` 0→n; `pos_y = 0`; `follow = 0` on all lines | no run, a second run, `trk=-1`, or a `follow=1` line |
| **G2 arithmetic** | the observed `(|Δ|, frames)` matches `T(s)` from the recorded seed (§3.4/§3.5) | a `Δ` matching no seed |
| **G3 offset** (Layer-1) | `lane_at_pass ≥ 1`, `(cam_dx,cam_dz) ≠ (0,0)`, `Δ ≤ 0xAB` on the live face | `lane = 0` (a zero-delta repeat — FU317-L10) |
| **G4 seed** | the gate's arm recorded (`has_slot`, dirs / `+0x73/+0x75`); `v0 ≥ 1` | `v0 = 0` (tracked-only, no motion) |
| **G5 pairs** | pan pair inside one displayed second **and** the delta run; clock-crop AE 0 | a pair across the second boundary |
| **G6 AE** | `AE_pan > AE_still`, both same-session | AE_pan ≤ AE_still |
| **G7 bbox** | recorded; sprite-only caveat declared (fail expected) | a whole-scene attribution from the bbox |
| **G8 invariants** | M1/M2 shas (§5.5); `goal_armed = 0`; OOB predicate 0; `make check` green | a sha move without §5 |

---

## 5. The M2 accounting (the faithful-move policy)

### 5.1 The two cases

- **Case A — drive only (FU320 GO on R2, no producer landed).** No production change ⇒ M1 and M2 must be **byte-identical**. The acceptance records both shas and the `cmp`; no re-pin, no diff, nothing to justify.
- **Case B — a producer lands (FU314-L5 `direct_face` and/or L3 `+0x7D` coverage).** The producer may move the sim *and* M2, **only** for a proven faithful reason (spec §5/§7: "a producer that changes the sim may move M2 with a written reason + frame diff; it may not be forced to keep M2 by an unfaithful staging"). Expect the move to be **large, not hash-only**: a live face turns every latched record, so the natural window's row-04 gate can start passing and the camera leave zero (§5.4 item 7) — the diff may span the natural frames, not just the staged m-42/link band. Procedure below.

### 5.2 The phase-31 precedent and the frame-diff format

**The no-move precedent (phase-31 T2), quoted** (`docs/ENGINE.md:5939-5947`):

> "**M2 `8434cfab…` byte-identical — no re-pin.** Reason (recorded, not assumed): the measured witness passes at the zero-delta point where the pre-T2 staged `0` and the live `0x100` both fold to Δ`0x80`; the row-0x24 call shapes coincide under the engine's staging (`player_e = 0` folds `(pos_x,pos_z)` and `(1,0)` to `0x100`/octant 2); and the one M2 spot where the old staged 0 was load-bearing — the m-42 band's staged south arrival — now stages its record **producer** input (`er->face7d = 0`, `tests/test_engine_m2.c:2862`), declared at the site and not a camera gate. The full-tape diff is empty."

That is the shape of a **faithful data-flow change that moves no frame**: the reason is *derived* (the fold at the measured point, the staging shapes), not asserted, and the one load-bearing fixture spot is re-staged as a **producer input**. The phase-32 producer slice must clear the same bar before it may claim Case A; if it cannot, it is Case B (below).

**The move precedent (phase-30 T2), quoted** (`tests/test_engine_m2.c:1697-1707`):

> "Phase-30 provenance (T2, `24da206` + `0414360`): the `+0x6B` lane-read family changed the chase's source word, moving **5,568 hash-only lines, frames 955..6522**. Every `frame=` id and `state=` tag in that range is unchanged (the first 954 lines are byte-identical; the line count stays 6,522 and no state stratum is added or removed); only the `hash=` column moves. This is the intended chase correction, not a regression. Re-pinned with the ISO present (`./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`); old M2 sha `b281a9b2…` -> new M2 sha `8434cfab…` (phase-30 final pin). M1 is unmoved (`09b726b7…`)."

The Stage-2 record reuses exactly those axes:

| field | required content |
|---|---|
| old → new sha256 | both full 64-hex, plus the byte count and the line count |
| first / last differing line | the line number **and** its `frame=` id (the test's `report_diff` prints the first offset, `test_engine_m2.c:6071-6092`) |
| the `frame=` range | the contiguous id range that moves |
| the moved columns | `hash=` only, or also `frame=`/`state=`/a stratum (the transcript's `state=` tags, asserted `:6030-6038`) |
| the `state=` strata | touched / added / removed (a new stratum is a red flag) |
| the anchors | whether the move crosses `m 41` (mech), `m 42` (`M2_M_BAND`), `m 58` (`M2_M_LINK`), or the v21 prefix (2 522 lines) |
| the intended consequence | the exact gate frames that flip and why (the face/bearing or the seed) |
| the collateral | every other moved frame classified, or "none" |

### 5.3 The written reason, the register pin, and the commands

The same commit carries: the code change, the golden, the **provenance block** in `tests/test_engine_m2.c` (the §5.2 wording, in the phase-30 style), the **register pin** in `docs/ENGINE.md` (the phase-32 entry: the shas, bytes/lines, the frames, the reason), `docs/PROGRESS.md` if a slice closes, and the spec's invariant line. The commands, in order:

```
./build/test_engine_m2 > /tmp/opencode/m2-new.txt        # ISO present, CWD = repo root
cmp tests/golden/engine/m2-frames.txt /tmp/opencode/m2-new.txt   # first diff (or let the test print it)
# analyse per §5.2; only if the diff is the intended consequence:
./build/test_engine_m2 > tests/golden/engine/m2-frames.txt
sha256sum tests/golden/engine/m2-frames.txt tests/golden/engine/m1-frames.txt
./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt   # M1 must not move
make check
```

The test itself refuses a mismatch (`memcmp` + `report_diff`, `test_engine_m2.c:6084-6092`) and needs the ISO for the golden comparison (`:6041-6046`); without the ISO the transcript is a different smoke mode, so **no `cmp` claim is valid without the ISO**.

### 5.4 The fixture hazards (the m-42 / link / additivity class)

1. **The m-42 band is already a non-zero-delta, face-aligned gate pass on the tape** (E21): park `(0,0,0xB00)`, record `(0,0,0xAF0)`, `lane 0x10`, `has_slot 1`, `face7d` staged `0`, slot dir `(0,+1)` → seed `+0x40` → `vel_z = 5`. The mover's face write is **next-frame** input, not same-frame: the record step loads the pool face (`match_run.c:1413`), dispatches the action (`:1441`/`:1457` — the row-04 gate reads *that* value), and only then runs the mover (`:1491`) whose `r->face7d = s.facing` (`:532`) is written back to the pool (`:1561`). So a live producer (`L5`) shifts the m-42 gate by one frame: the first staged dispatch still reads the staged `0`, later dispatches read the mover's output. With the fixture's record `+0x43 = 0` the turn is the identity (face stays 0). **The faithful shape keeps the m-42 gate alive:** the m-42 record's distance is `0x10 ≤ 0x60`, so the turn's close branch reads the `0x57A73` focus; with the focus staged to the camera triple (spec §9.2: it *is* in the native chain, and the default is the camera), `face_target = angle(cam − pos) = 0` = the bearing ⇒ Δ = 0 and the gate still passes. **The half-faithful shape is the one to fear:** if L5 stages `+0x43` live but leaves the focus at `(0,0)` (the port's current staging), the close branch sees `(0,0)` and lands on `face_target = facing` (identity, `action_handlers.c:219-231`) while a **far** target (`> 0x60`) rotates to the movement direction — so the tape's natural far records would turn and the m-42 fixture would not. That asymmetry is the first thing to check in the diff and the reason spec §9.2 forbids a half-faithful turn.
2. **The link fixture pins the pan itself**: `camera.pos_z == 0xB28`, `vel_z == 5`, `goal_armed == 1`, `zone 1` (E21). A seed/decay/arm change breaks those asserts loudly (a genuine RED, not a silent tape move) — good, but it means the producer slice must expect them.
3. **The m-42 fixture's staged `face7d = 0` is insensitive** (E22): it equals the pre-fix constant, so it cannot detect a re-staging regression. Do not use it as the producer's RED; the producer's own pin must show the live cell and the pre-fix behaviour failing (spec §5).
4. **The additivity gate is not a re-pin guard.** It compares the transcript against the *committed* golden's first 2 522 lines (`test_engine_m2.c:6063-6082`) — after a re-pin both are the same bytes, so the gate is tautological. The **manual prefix analysis** is the real guard: a move inside the first 2 522 lines is not an "additive extension" and must be argued in the reason.
5. **The face is a multi-consumer cell.** Beyond the half-line gate (`outfield.c:632`), `face7d` feeds the defender decision at `outfield.c:384` (a ±0x100 tolerance that installs row `0x0E`, `:582`), so a live face changes **AI decisions** as well as the gate. Those frames are collateral unless the diff shows them as the intended consequence of the same producer. This is the single largest M2-risk multiplier of the L5/L3 route.
6. **No staging may be converted into a gate.** The fixtures' staged `face7d`/`vel73/75` (`test_engine_m2.c:2862-2870`) stay declared **producer inputs**; a camera/face gate staged to force the tape is forbidden (spec §8.2/§9.1).
7. **A universal producer flips the natural-pass baseline asserts *inside* `make check`.** The phase-30 baseline is asserted in the test, not only in the golden: `camera_left_zero == 0`, `camera_zero_frames == granted_frames`, `first_left_zero_step == 0` (`test_engine_m2.c:3701-3707`), with the site's own note "the bootstrap producer will flip it and the P7/P8 window/lockout assertions land with it (FU313-L1)". If the live `+0x43` turns **every** latched record (the spec §2 datum: the port stages `0` so the turn "never rotates it"; the native byte is live), the natural window will pan and those asserts fail by design — the Stage-2 commit must invert them and land the P7/P8 window/lockout assertions in the **same** commit, as a declared test change, alongside the golden diff. This is the largest M2 move of the phase and it is not a golden-only event (FU321-L11).

### 5.5 The exact current pins (fresh this session)

| golden | sha256 | bytes |
|---|---|---|
| M1 `tests/golden/engine/m1-frames.txt` | `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` | 22 004 |
| M2 `tests/golden/engine/m2-frames.txt` | `8434cfab105b926e78279ca3ea03c62bd3a0ae9654d2e11f0930c0cb8605cee1` | 295 087 |

**Census:** `60/80` (37 action + 23 phase) — this slice wires no row and the four-doc same-commit rule (README title / `docs/ENGINE.md` / `docs/PROGRESS.md` / `AGENTS.md`) is not triggered. The README `75%` stays.

---

## 6. The hazards the Stage-2 plan must carry

1. **The controlled fallback (`is_ball_track`).** With `tracked == NONE` the port sets `is_ball_track = (id == controlled)` (E17), and arm B requires `is_ball_track == 0` (`outfield.c:682`). Consequence: **the controlled record cannot take its first pan in this build** — the witness must pan a **non-controlled** band record (the phase-31 witness panned record 2 while `controlled = 1`, `docs/ENGINE.md:6041-6049`). FU320's drive must be checked against this; a drive that only moves the controlled record yields a tracked-set/no-pan run. Mitigation: select a non-controlled band record, or land FU314-L4 first (out of this phase).
2. **The claim ordering.** The event is `f=1513`, the first motion `f=1514`, the claim (`ball.carrier`) `f=1514`; the pan is **upstream** of the claim (E16, `docs/ENGINE.md:6053-6062`). The acceptance must not attribute the pan to the claim, must not require the claim (`FU315`: the claim is optional for the pan), and must not assert a claim at all when the panning record is not the possession record (FU321-L9).
3. **The one-shot and the row-6 diversion.** The pan's own tail installs row 5 (`outfield.c:719`) ⇒ the same record cannot re-pan without a re-entry; the row-6 arms (`:709-716`) can divert (no claim). Pin `pan_segments == 1` and `row04_after_event_frames == 0` (the T1 pins), and do not claim a second pan.
4. **The knife edge is *moved*, not removed.** The phase-31 pass was knife-edged on `lane == 0` (FU317-L10). The new witness replaces that with two sharper edges: (i) the **band-entry frame** must be a *passing* frame — the angle must hold at one of the `lane ∈ [1, 0x40]` frames, on the approach (the `lane ≤ bound` term fails on an exit), and (ii) the drive's timing selects which frame that is. The acceptance must record the gate frame, not assume it.
5. **The drive timing (interactive).** The live run is not the null tape: phase 2 at tick 1897 vs 1802, and the tape's prefix is not hand-reproducible (`p31-l2-notes.md:72-84`). FU320's drive starts **phase-2-relative**, and the capture keys on the trace's first motion, never on a wall clock. The bare hold **fails** (the negative run) — the prefix is mandatory.
6. **The sprite-only bbox.** Gate 2 cannot pass in this build (E23); the AE is relative only. Never let the bbox or the AE stand in for the trace.
7. **A period-staged witness (only if FU320 proves R2 needs it).** Allowed by spec §9.3 as a **declared match-setup input**, never a camera/face gate; the note must say what was staged, why, and that the drive still starts from the kickoff. If the second half is the only route, the staged period is part of the witness definition.
8. **The zero-seed silent event.** `s = 4·w` with `|w| ≤ 2` ⇒ `v0 = trunc(s/0xC) = 0` ⇒ `fifa96_camera_event_set` succeeds, `trk` binds, and the walk block never runs (`camera.c:86`: `speed == 0 && event_param == 0`) ⇒ **no motion, no pan counter, no changed frame**. The trace looks "almost right" (a bind) and the AE still passes (the scene animates). This is the most dangerous false pass of the phase; the fail rule is G4 (`v0 ≥ 1`) + G1 (a delta run). The m-42 fixture stages exactly this seed shape (`vel73/75 = 0`, E21) — the tape's own precedent for a zero seed.
9. **The trace cannot show the offset (the honesty boundary).** Layer-2 reproduces the drive and the arithmetic; the non-zero-delta tuple is a Layer-1 citation (G3). The register wording must be "a pan reproduced live under the non-zero-delta drive", not "the trace shows a non-zero-delta pan".
10. **The `Z = 0` wording.** The phase-31 `Y=Z=0` is the DOWN drive's property (§3.7); a mirrored or z-axis drive moves Z. Assert `pos_y = 0` always and `pos_z` = the seed's `T(s_z)`.

---

## 7. The port shape (what the Stage-2 plan carries)

**If FU320 returns R2 GO (the preferred route):** the phase lands a **drive-only witness**.

- Layer 1: extend the existing natural pass in `tests/test_engine_m2.c` (the T1 pass, `:4007-4235` per the register) with FU320's drive, and add the **offset pin** G3/G4 (§4.6) — the gate snapshot records `lane_at_pass`, `(cam_dx,cam_dz)`, `has_slot`, dirs/velocity words, `face7d`, `Δ`. No production code, no camera/face staging. The T2 fixtures (`test_row04_live_face_passes_face_aligned_gate` / `…_zero_face_fails…`, carried `docs/ENGINE.md:5933-5937`) stay as the unit pins; the new acceptance fixture stages producer inputs only.
- Layer 2: `docs/screens/p32-l2-notes.md` + the five artefacts (`p32-l2-pan-a/b.png`, `p32-l2-still-a/b.png`, `p32-l2-pan-diff.png`) + the trace extract `p32-l2-trace-pan.txt` (the phase-2 line, the pan window, the still window), per §4.
- Invariants: the M1/M2 `cmp`/shas (Case A), the census, `make check`.

**If FU320 returns NO-GO and FU318/FU319 return GO:** the producer slice lands first, in this order: (1) the minimal faithful staging (the §5.4 checks first — the m-42/link fixtures are the early-warning asserts); (2) the producer's own pin (the live cell + the pre-fix RED); (3) the **M2 frame diff** + the written reason + the register pin (Case B); (4) only then the drive + the Layer-2 acceptance. If the diff is not faithful, **stop** (spec §6 honest-stop) and record the leg.

**Files touched by the eventual slice:** `src/fifa96_engine/fifa96_match_run.c` (the mover staging `:498`/`:532`), possibly `src/fifa96_loader/fifa96_action_handlers.c` (the turn's inputs), `tests/test_engine_m2.c` (the pass + the provenance), the frame-path pins, `docs/screens/p32-l2-*`, `docs/ENGINE.md`, `docs/PROGRESS.md`. No new target, no CMake change, no `tools/` change.

**Cost:** the Layer-1 pass is one extra 12 000-step run (~9 s, FU317-L8) or a shorter focused run; the Layer-2 is one `make game` + a 70 s clip + the extracts; the producer slice is the only code cost and its M2 analysis (§5) is the expensive part.

---

## 8. Numbered legs

1. **FU321-L1 — the trace cannot prove the gate offset.** The trace's fields are camera cells only (E13); no record `pos`/`lane`/`face`/bearing. The non-zero-delta claim is therefore Layer-1, and Layer-2 can only reproduce the drive + the arithmetic. Owner: the Stage-2 acceptance note; a future record-cell trace is a new leg.
2. **FU321-L2 — the zero-seed silent event.** `|w| ≤ 2` ⇒ `v0 = 0` ⇒ an event with `trk` bound and no motion (E3/E9, §6.8). No existing pin forbids it; the acceptance's G4 (`v0 ≥ 1`) + G1 (a delta run) is the only guard. Owner: the Layer-1 probe (record the seed) + the drive selection.
3. **FU321-L3 — the no-slot `T(4w)` instances are formula-only.** The table (§3.5) is derived from the code and the stride flat's `|max| = 15`; only `w = 0` (the m-42 staged zero) and the slot arm's `0x40` are tape/measurement instances. Arm-written transients outside the stride domain are unenumerated. Owner: the Stage-2 Layer-1 probe (record `+0x73/+0x75` and the observed run).
4. **FU321-L4 — the new drive's interactive timing is unmeasured.** The T3 ticks (phase 2 1897; gate/event/motion 5044/5047/5050; keyup 5270) belong to the DOWN drive; FU320's drive maps to the live run only by a phase-2-relative start, and the live↔null `f` alignment must be re-measured. Owner: the Layer-2 acceptance task.
5. **FU321-L5 — the m-42 fixture's re-staging insensitivity.** Its staged `face7d = 0` equals the pre-fix constant (E22), so it cannot be the producer's RED; the producer pin must be a separate frame-path fixture. Owner: the Stage-2 producer plan.
6. **FU321-L6 — the additivity gate is not a re-pin guard.** It is tautological once the golden is regenerated (§5.4 item 4); the manual prefix analysis (the first 2 522 lines, the v21 spine) is the only guard against a "re-pin as extension". Owner: the Stage-2 M2 accounting.
7. **FU321-L7 — the visible shift scales with the seed.** The `≈50 px/320` datum is for `Δ = 0x90`; a `0xC`–`0x84` pan shrinks the AE margin and the clip pair window (0.2 s at 6 frames). The relative gate still holds, but the note must record the expected shift band, not the phase-31 numbers. Owner: the Layer-2 acceptance.
8. **FU321-L8 — the period-staged witness (conditional).** Only if FU320 proves the second half/restart is the only R2 route; declared as a match-setup input (spec §9.3), never a gate. Owner: FU320/Stage-2.
9. **FU321-L9 — the claim may be absent.** For a non-possession pan record, `ball.carrier` does not change and the row-05 claim does not fire; the acceptance must not assert a claim (the phase-31 witness's claim is that drive's, not the pan's requirement). Owner: the Stage-2 acceptance wording.
10. **FU321-L10 — the face is a multi-consumer cell (the strongest M2 collateral).** Beyond the half-line gate (`outfield.c:632`), `face7d` feeds the defender decision (`:384`) and its row-0E install (`:582`), so a live face changes AI decisions; the diff must classify those frames. Owner: the Stage-2 M2 analysis (FU319's diff).
11. **FU321-L11 — the natural-pass baseline asserts flip with a universal producer.** `test_engine_m2.c:3701-3707` pins the zero camera as the phase-30 witness and declares that "the bootstrap producer will flip it"; the P7/P8 window/lockout assertions must land in the same commit as the inversion, or `make check` stays RED for a legitimate producer. Owner: the Stage-2 producer slice + the M2 accounting.

---

## 9. GO/NO-GO

**GO — the acceptance design and the M2 policy.** The pan arithmetic is closed-form and reproduces **five** independent published instances exactly (§3.4: `0x90/0x140/0x174/0x1FA/0x3B8`); the Layer-2 procedure is the frozen, already-executed T3 procedure (§4) with FU320's drive substituted; the M2 policy is the spec's faithful-move with a concrete diff format, the exact commands and the known fixture hazards (§5). None of it requires a new native port and none of it stages a camera/face gate.

**The Layer-2 GO is conditional on FU320's tuple**, with two binding conditions:

- **(C1) non-zero offset at the passing dispatch:** `lane_at_pass ≥ 1` ⇔ `(cam_dx,cam_dz) ≠ (0,0)` ⇔ `pos ≠ cam` (§3.8). If the only passing frame is `lane == 0`, the witness is the phase-31 zero-delta repeat — **NO-GO on FU314-L1a**.
- **(C2) a non-zero seed:** the gate record's arm yields `v0 ≥ 1` (§3.6/§6.8). A zero seed binds `trk` and moves nothing — **NO-GO**.

**Case B carries one more gate:** the natural-pass baseline inversion (L11) must land with the P7/P8 window/lockout assertions in the same commit; deleting or weakening the zero-camera asserts without the replacement window is a **NO-GO**.

**NO-GO on:** (a) citing the trace (or the AE, or the bbox) as proof of the offset — the offset is Layer-1 (L1); (b) accepting a tracked-only / no-delta-run event as a pan (L2); (c) any M2 move without the frame diff + written reason + register pin in the same commit (§5); (d) any staging of a camera/face gate, or a period staging not declared as a match-setup input (§4.1, §6.7).

---

## 10. Known limits

1. **No measurement in this slice.** No probe was run (the plan's Task 4 needs none): the `T(s)` table is code-derived and cross-checked against FU316's published values, not re-measured. FU320's tuple (record id, gate frame, seed) is an **interface**, not a number in this doc.
2. **The drive is FU320's.** The live ticks in §4.2 are the T3 DOWN-drive's; the new drive's null→live mapping is a leg (L4) and its keys are not invented here.
3. **The no-slot instances are formula-only** (L3); the arm-written transients outside the stride domain are unenumerated.
4. **The M2 diff cannot be predicted without the producer's shape** (FU319's verdict); §5 defines the procedure, not the numbers. The largest risk is the multi-consumer face (L10), not the gate itself.
5. **The visible shift is computed** from the FU317 §3.7 datum for `0x90`; a smaller pan's shift and AE margin are proportional estimates (L7), and Gate 2 remains impossible (sprite-only).
6. **The trace's `f=` is run-relative** and the live phase-2 landing drifts; only same-run pairs and same-drive anchors are valid.

---

## Review erratum (wave review, 2026-10-22)

**1. The seed criterion is absolute, not signed.** Quote (§1, §9 C2, §4.6 G4): "a seed with `v0 ≥ 1`" / "`v0 ≥ 1`". **Corrected:** the seed is signed — the nominated FU320 drive is `dir = (−1,0)`, seed `−0x40`, `v0 = trunc(−0x40/0xC) = −5` — so the binding criterion is **`|v0| ≥ 1`** (equivalently the event's `speed > 0`, `s ≠ 0`). As written, C2/G4 would reject the intended witness. Every other criterion stands.

**2. C1/C2 are satisfied by FU320's nominated drive.** Quote (§9): "**The Layer-2 GO is conditional on FU320's tuple**". **Fresh (FU320 §4.2/§6):** the nominated drive is `M2_KEYS` verbatim + **LEFT 1820..1849** + **DOWN 1850..end**; at the passing dispatch (trace key `f=1528`) the tuple is `pos (−32,0,16)`, `lane 36`, `bound 36`, `cam_dx +32`, `cam_dz −16`, `face7d 0x100`, **bearing `0x14C`, Δ `0x4C`**, `has_slot 1`, `dir (−1,0)`, `tracked −1`, `controlled 1`; the seed is `−0x40` → `|v0| = 5`. **C1 holds** (`lane = 36 ≥ 1`, `(cam_dx,cam_dz) ≠ (0,0)`) and **C2 holds** under the corrected absolute form. The measured pan is the slot-arm class: event `f=1529`, motion `f=1530..1556` (27 frames), stop `f=1557` at `(−0x90,0,0)`, `pan 0→4`, Y=Z=0, `follow=0` (3 598 trace lines); the acceptance reads the observed `(|Δ|, frames) = (0x90, 27)` and states the offset only from the Layer-1 tuple (G3). The Layer-2 GO is therefore unconditional for this drive, subject to the live-timing leg **FU321-L4** and the pair/AE/bbox rules (§4.3–§4.5).

---

## 11. Provenance and self-audit

**Fresh this session (repo reads, no writes):** `src/fifa96_loader/fifa96_outfield.c` (`:360-388`, `:400-420`, `:560-721`), `src/fifa96_loader/fifa96_camera.c` (`:1-151`, `:640-830`, `:900-1000`), `src/fifa96_loader/fifa96_entity_update.c` (`:60-139`), `src/fifa96_loader/fifa96_action_handlers.c` (`:180-419`), `src/fifa96_loader/fifa96_match_state.c` (`:30-89`) + `include/fifa96_loader/fifa96_match_state.h:6-7`, `src/fifa96_engine/fifa96_match_run.c` (`:480-559`, `:1395-1424`, `:1470-1549`, `:1826-1855`, `:4340-4476`, `:4570-4633`), `src/fifa96_engine/fifa96_match_handlers.c` (`:2600-2819`), `src/fifa96_engine/fifa96_match_entities.c` (`:278`, `:315-316`, `:758-782`), `src/fifa96_engine/fifa96_match_locomotion_tables.c` (the stride flat, parsed for `max |value| = 15` over 0x500 words), `tests/test_engine_m2.c` (`:1690-1775`, `:2795-2894`, `:3690-3719`, `:6030-6099`), `Makefile:39-41`, `docs/superpowers/specs/2026-10-22-fifa96-m2-phase32-face-aligned-pan-design.md` (§2/§5–§9), `docs/superpowers/plans/2026-10-22-fifa96-m2-phase32-recon-ahead.md` (Task 4), `docs/ghidra/FU314_r2l6_pan_triggers.md` (§2.1-§2.3, §6-§7, the errata), `docs/ghidra/FU316_r2l6_pan_walk.md` (§4.2-§4.3, §5.1, §6), `docs/ghidra/FU317_r2l6_claim_acceptance.md` (§3.1-§3.4, §5, §7), `docs/screens/p31-l2-notes.md`, `docs/ENGINE.md:5860-6160`, `AGENTS.md`.

**Fresh hashes this session:** M1 `09b726b7…a2cd` (22 004 B), M2 `8434cfab…ce1` (295 087 B) — matching the register's pins (`docs/ENGINE.md:6155-6156`).

**Carried native facts (marked):** the gate bytes `0x7ED89..0x7EE7D` and the arm bytes `0x7EF42/0x7EFEE/0x7F003/0x7F035/0x7F056` (FU314 §2.2), the ramp/idle constants `0x70544` and the decay `0x15781B = 24` (FU316 §4.2), the armer gate `0x7131C` (FU316 §5.1), the angle map (FU314 §9). No native byte was re-read; no claim in this doc depends on a fresh native read.

**Self-audit:** every numeric claim above is either (a) a fresh `file:line` of the port/test source, (b) a formula explicitly derived from those lines and cross-checked against a published value (§3.4 reproduces four FU316 values + the measured one), or (c) a *carried* native byte with its owner doc named. The unproven items are numbered: the live timing (L4), the no-slot instances (L3), the producer's M2 diff (L10/§5), the visible shift for small pans (L7), the period-staged route (L8), the trace's blindness to the offset (L1), the natural-pass baseline flip (L11). No number in §3–§5 is asserted from recollection.
