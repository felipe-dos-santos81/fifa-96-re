# FU-333 — the `+0x79` producer's acceptance and the natural no-slot pan (phase-35 R2-L9)

**Phase/Provenance:** M2 phase-35 Stage-1 recon-ahead wave, **Task 4 of 4**
(`docs/superpowers/plans/2026-10-23-fifa96-m2-phase35-recon-ahead.md:84-99`),
serving `docs/superpowers/specs/2026-10-23-fifa96-m2-phase35-placement-timer-design.md`
§5 (Structure), §6 (Acceptance), §7 (honest-stop), §8 (Decisions). **Repo-static
design: no Ghidra read, no probe, no build, no measurement.** Every native fact
is **carried** with its owner (FU330 §2/§3 — which landed mid-draft and is cited
as the fresh owner of the producer's form — FU84 §6, FU328 §5.4/L1–L3, FU326
§4.2, FU327 §2/§4); the port's producer design is FU331 §2/§3 (also landed
mid-draft); every quantity the measuring slice owns is a **forward-cite** to
**FU332** (the measured blast radius, the natural tuple, the A3 pacing) —
written as *the measured value*, never a number. Every port/test claim is a
first-hand `file:line` read this session (§12). Fresh this session: the M1/M2
`sha256sum`/`wc`/`grep -c` pins (§6.5). The only repo write is this draft; no
git; the ISO, `build/` and `tests/golden/` untouched; no probe; scratch
`/tmp/opencode/fu333/` unused (nothing needed a binary).

---

## 1. The question

> Given the spec §5–§8, FU329's Branch B, FU328-L8's golden exposure and the
> phase-33 faithful-move precedent: exactly what does Stage 2 accept, with which
> cells and in which commit, once the `+0x79` producer makes the no-slot arm
> carry a real seed — the Layer-1 witness tuple and its pin classes, the
> no-staged-cell rule, the drive naming, the A3 window's re-pin and its commit,
> the golden's faithful-move recipe (the first-diff anchor, the `state=` check,
> the stratum-set gate, M1), the P1/P2 split, and every hazard — while
> asserting no number FU330/FU331/FU332 own?

## 2. The one-page answer

**The design is GO in both branches; the witness is conditional on FU332, and
the golden move is unconditional — it is P1's (the producer's) own commit.**
Phase 35 is the first phase since phase 33 whose *expected* outcome is a golden
move: `+0x79` is produced natively (FU330's census: the `0x10F394` per-player tier
table OR `[rec+0x9D]`, written in the placement prelude; the port's own produced
value is **6** on all 22 records while `player_d` is staged 0 — a port value, not
a native per-record measurement, which stays FU330-L1/FU328-L3), the placement
copies it into `+0x7B` (`fifa96_match_entities.c:94/209/744`), the mover's
stride nibble rises (`fifa96_match_run.c:549`), and every record's velocity
words and place follow (`fifa96_action_handlers.c:257-281`). **P1** is the
atomic producer commit — the producer, the row-09 OL-82 interplay, the unit
pins, the natural-pass *survival* re-pins (A3 included), the driven-pass
re-pins, the M2 re-pin with the faithful-move accounting, the FU324-L5
re-measure record, and the register/pin — the phase-33 T1 shape
(`docs/ENGINE.md:6729-6732`). **P2** is the acceptance: the natural no-slot
witness pins (Branch A) or the measured distribution + legs (Branch B), both
**test-side only**, and therefore **cannot move M2** — the natural pass writes
no transcript (`tests/test_engine_m2.c:3858-3907`, `:5146-5163`; the golden is
`run_scenario`/`run_tape`'s, `:3714-3718`, compared at `:7021-7095`), so P2's
byte-identity is a gate by
construction (FU329 §6.1's Case A, carried).

The witness is FU329's Branch B concretized on the **undriven `M2_KEYS` tape**
(verbatim, 12 000 steps, no added hold, no staged slot/seed/face/timer cell):
a row-04 pass on the band record, `has_slot 0`, the `0x7F056` arm (seed
`= (int16)(vel73 << 2)`, not `dir << 6`), a **nonzero produced chain**
(`timer79` live at the gate, the nibble ≥ the measured minimum, `|vel| ≤ nibble`),
the gate terms on the live face, a moving event pair (`|v0| ≥ 1`), and a real
delta run of the curve's frames/place with `pan 0→n`. **Every value is FU332's
measured tuple; this doc writes only the slots, the pin classes and the fail
signatures.** If FU332 measures no tuple, P2 is the measured-distribution
register and the legs — no fixture, no drive, no staging (spec §7; §8.3).

**The A3 re-pin is the ruling this doc exists to make.** The phase-33 inversion
window `M2_NAT_FACE_FIRST_LO/HI = 4930/5200`
(`tests/test_engine_m2.c:1948-1949`, A3 `:3972-3979`) is anchored on the
*producer-less* first camera departure (step 5065; `docs/ENGINE.md:6812-6815`).
With `+0x79` produced, the band gate is expected to move **earlier** (the
diagnostic's sub-native ladder already fires at engine steps 2899/2355/2032 for
`+0x79` = 3/6/15, FU328 §5.6, well before 5065), so A3 is expected to fail the
moment P1 lands. **A commit that re-pins M2 may not
leave `make check` red** (AGENTS.md's gate; spec §8.1's own RED/GREEN), so the
A3 re-pin **lands in P1** with FU332's measured step and a declared slack — the
spec §6 sentence "the A3 window re-pins in P2" is read as *the values are the
measurement's*, and its commit placement is corrected by a numbered leg
(FU333-L2) for the Stage-2 spec erratum. P2 *verifies* A3 (a moved value after
P1 is a stop, never a second re-pin). If FU332's measured step still lies inside
4930/5200 (with the port's produced value 6, the diagnostic's step-2355 row makes
this very unlikely), P1 records the verified no-op and re-pins nothing — the rule
is "whoever turns it red re-pins it, once".

**Invariants both branches close on:** M1 `09b726b7…` immutable (`cmp`); M2
re-pinned only in P1 under the recipe (§6); census `60/80 = 75%` and the README
title untouched (H9); `make check` green at every commit.

### 2.1 The two branches at a glance

| | Branch A — FU332 measures the tuple | Branch B — it does not |
|---|---|---|
| P1 | the producer + the golden move + A3 + the survival re-pins (§4.6, §5, §6) | identical — the producer is native fidelity, not an acceptance |
| P2 | the natural witness pins (§4) | the measured-distribution register + the legs (§4.7); no test change |
| Golden | P1's faithful move; P2 byte-identical by construction | identical |
| Layer 2 | the undriven live run (§7) | not attempted (no tuple to reproduce) |
| Legs | FU333-L1…L15 | same, L1/L12 narrowed |

---

## 3. The bindings

| # | Binding (owner cite) | What it fixes here |
|---|---|---|
| B1 | spec §5 (the P1–P4 sketch) / §6 (acceptance, NO-GO, golden governance) / §7 (traps) / §8 (decisions 1–6) | the two-commit split, the acceptance's shape, the stop rules |
| B2 | FU329 §4 (the tuple, the split, the pin surfaces, the naming grammar), §6.2 (the faithful-move recipe), §8 (H1–H10), §9 (the legs), erratum 4 (the producer is the future, golden-moving, separately pinned fix) | the witness design to concretize; the recipe's house form |
| B3 | FU328 §5.1/§5.6 (the measured baseline + the labelled diagnostic), §8 L1/L3/L8 (the producer gap, the unmeasured native value, the wholesale M2 exposure), erratum 2 (`+0x79` = table **entry** range OR `[rec+0x9D]`) | the exposure, the diagnostic's status, the OR hazard |
| B4 | **FU330** §2/§3 (fresh: the producer is the 32-byte per-player tier table `0x10F394`, indexed by the roster byte `[[rec+4]+0xD]`, OR `[rec+0x9D]`; three writers P1/P2/P3; the store at `0x79A4D` in the placement prelude; the port's produced value `6` while `player_d` is staged 0) + FU84 §6 (the carried `FUN_0007997C`/`0x10F394` material incl. the phase-0C `>>1` and code-24 `-2` `+0x7B` readers) | the producer's form, its timing and the `+0x7B` writer class. **FU330's fresh index byte `+0xD` supersedes FU84 §6's `+0xA`** (FU84 is append-only; the correction is a wave-review erratum) |
| B5 | FU326 §4.2 (`\|w\| ≤ 15`, `s = 4w`, `v0 ∈ [−5,5]`), §4.3 (`T ≤ 0x90` per axis), §4.4 (the `\|w\| ≤ 2` silent band) | the seed domain and the silent-band control |
| B6 | FU327 §2/§4 (the five measured gate passes; the formation-wide `max \|w\| = 1` on the pre-producer tree) | the pre-producer control the witness must *differ from* |
| B7 | the phase-33 register (`docs/ENGINE.md:6715-6750`, `:6775-6797`: the 9/52/852 chain, the 5 671 hash-only lines, Outcome B) + FU322's erratum 5 + FU323 §6 (the recipe, the written reason) | the faithful-move precedent and the house form |
| B8 | the phase-34 register (`docs/ENGINE.md:7398-7415`, esp. `:7406-7407` "the future A3 re-pin rides that producer, not this phase"; `:7489-7504` the promoted FU328-L1) | the hand-off this slice receives |
| B9 | `tests/test_engine_m2.c`: the A3 constants/comment `:1939-1949`, A1–A3 `:3962-3979`, the natural structs `:2572-2627`, the sampler `:3754-3852`, the natural pass `:3854-3935`, the probe `:5146-5163`, the gate ring `:2442-2457`/`:4219-4257`, the segment record `:2487-2499`, the curve `:2467-2481`, the stratum-set gate `:7021-7089`, the compare `:7090-7098`, the provenance block `:1716-1750` | every pin surface this design touches |
| B10 | `src/fifa96_engine/fifa96_match_entities.c:94/209/744`, `fifa96_match_run.c:549`, `fifa96_loader/fifa96_action_handlers.c:257-281`, `fifa96_match_handlers.c:5034-5043` / `:5785` / `:5813-5822` / `:5846` / `:2344` / `:4861`, `fifa96_loader/fifa96_arm_bodies.c:51-52` | the chain and the `+0x7B` writer census this design must respect |

---

## 4. The Layer-1 acceptance (Branch B concretized)

### 4.1 The witness slots (values are FU332's)

On the undriven natural tape (`M2_KEYS` verbatim; `M2_NATURAL_WINDOW_STEPS`
12 000; ISO present), a row-04 pass whose granted frames are the sampler's own
three-frame ring — gate `f−2`, event `f−1`, first motion `f`
(`:4287-4288`'s convention, `:2442-2457`'s snapshot) — such that:

| slot | cell (at the ring) | pin class | why |
|---|---|---|---|
| **W1 the row** | `code == 4`; `anim_id != 0x13`; `cam_pos_y == 0`; `has_slot == 0`; `\|cam_pos_x\| ≤ 0x630`, `\|cam_pos_z\| ≤ 0xA20`; `tracked != id`; `controlled != id` | asserted, exact | the `0x7F056` arm is a *conjunction*, not `has_slot` alone (FU329 H3; spec §6) |
| **W2 the gate** | `lane_x`, `lane_z`, `cam_dz6f`, `bound`, `timer81`, `face7d`; `lane_x ≥ 1`; `lane_x ≤ 0x40 − timer81`; `lane_x ≤ bound`; Δ = fold(bearing − `face7d`) by the port's own `fifa96_entity_angle` + the exact fold, `≤ 0xAB`, on the **live** face | cells exact; predicates structural | the half-line gate set (FU316 §3.1); `lane = 0` is NO-GO (spec §7) |
| **W3 the producer chain** | `timer79` (the produced limit), `timer7b` and its `& 0x0F` nibble, `vel73`, `vel75`, `speed71`, `pos`, `target`; the nibble ≥ the measured minimum (and ≥ 3 for a seed ≥ 0xC by the algebra); the consistency `\|vel73\|, \|vel75\| ≤ (timer7b & 0x0F)` | cells exact; the bound structural | *this* phase's new dimension: the chain must be visible, or the witness is the arm with an unexplained seed (H1/H2) |
| **W4 the event** | `cam_vel_x/z` = the seed-derived pair, `speed`, `timer`/`timer_limit`, `ramp_divisor`, `event_param`, `tracked == id` | exact | the fast path `v0 = trunc(s/0xC)` while the pair's metric ≤ `0x19`; above the cap the measured pair is the authority (FU329 H5, carried) |
| **W5 the run** | `first_motion_f`, `stop_f`, `changed_frames`, the stop place, `pan`/`pan_max`, `speed_seq`, `pos_y == 0`, `speed == fifa96_entity_distance(vel_x, vel_z)` per frame, the monotone signs = the seed's | structure asserted from the curve; values exact | `changed_frames == max(fr_x, fr_z)` and the stop `(T_x,0,T_z)` are `m2_pan_curve`'s outputs (`:2467-2481`), never a literal |
| **W6 the tail** | `row04_after_event_frames == 0`; the segment's later windows recorded | exact / recorded | the one-shot (FU316 §3.2) |
| **W7 the honesty** | the whole-run segment list, every changed-place window, the frequency, the in-run zero-seed control | recorded only | the p32 half-time-reset rule + FU329 N-W6; `off_zero_frames` is never the run count |

**The tuple is not written here.** The frame ids, the record id (= the bind's
`tracked`), the nibble, the seed words, the event pair, the per-segment
frames/place and the pan are **FU332's measured tuple**, re-run once for
stability (the plan's Task 3 Step 6). "≥ 3" above is the algebra's threshold
(`|w| ≥ 3` → `seed = 0xC` → `v0 = ±1`; FU328 §3.2), not a measured value; the
witness pins the measured nibble literally and the threshold structurally.

### 4.2 Why W3 is new and non-negotiable

The phase-34 design had no producer, so it sampled the seed indirectly (the
event's `cam_vel`, FU329 N-W3/FU329-L3). Under the producer the *chain* is the
claim: a witness that shows `|v0| ≥ 1` while the band record's `timer79` /
`timer7b` / `vel73` cells are zero or absent is indistinguishable from a staged
or accidental seed. Three consequences:

1. the gate struct must grow the cells (`vel73`, `vel75`, `timer79`, `timer7b`,
   `speed71`, `target_x/z`) — the FU329-L3 addition, now mandatory, and the
   `m2_claim_snapshot` form (`:4219-4257`) parameterized for
   `M2_NAT_BAND_ENTITY` (`:1938`) rather than `M2_CLAIM_ENTITY`;
2. the natural pass needs a **ring**, not a single sample (today
   `m2_natural_sample` keeps no history, `:3754-3852`), so the gate/event/motion
   frames are the *same run's* post-frame cells at `f−2/f−1/f`;
3. the natural delta-run machine (`:2572-2591`) keeps only the **last**
   segment's `first_motion/stop` (`:3810-3816`); P2 adds a per-segment record
   (the `m2_claim_segment` shape, `:2487-2499`, incl. `speed_seq`) so the
   witness segment is named, not inferred from the pass-level cells (the
   phase-33 seg[1] defect class, FU322's erratum 2).

### 4.3 The recorded-vs-asserted split (FU329 §4.2, carried)

| class | cells | rule |
|---|---|---|
| **pinned exact** | W1/W2's arm and gate cells, W3's chain cells, W4's event cells, W5's run cells | literal from FU332's tuple; the null tape is deterministic (`run_natural_probe` replays twice and `memcmp`s, `:5146-5150`) |
| **asserted structural** | `\|v0\| ≥ 1`; `segments ≥ 1`; `lane_x ≥ 1`; Δ ≤ `0xAB`; the frame order; the nibble bound; `pos_y == 0`; the one-shot; the A3 clause `first_left_zero_step > phase2_step` | predicates, never frame literals |
| **recorded only** | the exact `f`/`step`/`rel` of every anchor; `camera_zero_frames`/`camera_left_zero`; the bearing/Δ; `off_zero_frames`; every later segment; the sample hash; the frequency; the achieved `(\|seed\|,\|v0\|,T)` | never asserted; cited as measured |

A pin that freezes a *measurement* instead of its *shape* turns the next faithful
producer into a RED (the phase-33 inversion lesson, FU321-L11) — hence the
split, exactly as FU329 §4.2.

### 4.4 The no-staged-cell rule (the source-level form)

The natural pass stages **only** the copied tape (`:3862`) and the keeper-ladder
observation seam (`:3905-3906`). The acceptance adds no staging, and P2 must
prove it, not assume it:

1. the P2 diff touches `tests/test_engine_m2.c` only, and adds **no write** to
   `mr->entities`, `mr->record` or `mr->render.camera` in `run_natural_pass` /
   `m2_natural_sample` (a diff inspection, recorded in the commit body);
2. a post-run `memcmp(tape, M2_KEYS, sizeof M2_KEYS) == 0` on the pass's copy
   (the null platform consumes the copy, so a mutation is detectable — a cheap
   honest check);
3. no new staging `#define` (`+0x79`, `+0x7B`, `has_slot`, `vel73/75`, `face7d`,
   camera) appears in the natural path; the producer's values are the engine's;
4. the diagnostic ladder (FU328 §5.6: `+0x79` staged 3/6/15) stays **out of the
   tree** — it is causal evidence, never a witness (spec §8.3), and P2 must not
   inherit its `--stage-t79` shape;
5. a witness that needs a staged cell is a leg + NO-GO (spec §7).

The in-test control for "the engine, not the fixture, produced it": the band
record's `timer79` at the gate is non-zero and equals the value the limit's own
producer wrote (the entity reset; `timer79` has one producer class), while
`+0x7B` has several writers (H6), so the counter is checked for *consistency*
with the mover's read (W3's bound), not for equality with the limit. A mismatch
is a **stop**, not a tolerance.

### 4.5 The drive naming

```
drive   := { tape: M2_KEYS verbatim, steps: 12 000, keys added: none }
anchor  := { f: the granted-frame index, step: the engine step, rel: step - phase2_step }
witness := "the undriven tape; record id <FU332>; gate f = <FU332> (rel <FU332>);
            event f+1; first motion f+2; stop f = <FU332>"
```

1. Every anchor is named in `f` **and** `step` (the A3 constants are *steps*,
   the sampler's cells are *frames*; the mapping is not linear — 3 598 grants
   over 11 996 live steps — so a conversion by ratio is forbidden).
2. The record id is read from the bind (`tracked`), never assumed
   (`M2_CLAIM_ENTITY`'s convention, `:1912`; the band constant `:1938` is
   asserted *equal* to the bind).
3. The tape's keys are named by tape index, never by absolute tick (the
   p31/p32 convention, FU329 §4.4 item 2).
4. The arm is a cell (W1's `seed == (int16)(vel73 << 2)`), not an inference
   from `has_slot` alone.
5. The diagnostic's frames (867/704/607 at the sub-native 3/6/15) are **never**
   cited as the witness's frames (FU328 §5.6).

### 4.6 The survival re-pins that ride P1, not P2

P1 turns the producer on for **every** run in the binary, so these existing
asserts are P1's responsibility (the phase-33 T1 precedent carried the
segment-aware re-pins in the atomic commit, `docs/ENGINE.md:6729-6732`):

| # | surface (fresh) | what moves | rule |
|---|---|---|---|
| S1 | A3 `:3972-3979` + the constants `:1948-1949` | the first camera departure step (earlier) | re-pin with FU332's measured step + the declared slack (§5) |
| S2 | `assert(nat->granted_frames == 3598u)` `:3944-3947` | nothing expected (the pace is clock-driven) | verify once; **a moved count is a stop** |
| S3 | `assert(nat->stage.calls == 6118u)` + `select_ran == 0` `:4003-4010` | the keeper seam is AI-coupled (the FU322-L8 class) | re-value once from the measured build; `select_ran == 0` should survive structurally (the pan stays inside ±`0x90`, far from `\|cam_z\| > 0x7B0`) |
| S4 | the natural score negative `:3910-3913` (`score 0-0`, `score_last_side −1`, `max_diff 0`, `last_event 0` at **every** sample) | a moving formation can score — the T3 negative is movement-coupled | **a RED here is a stop-and-report**, not a loosened assert: if the natural tape scores under the native producer, the natural window's shape changes and the phase re-scopes (§8/H3) |
| S5 | `assert(nat->kick_step - nat->match_start_step <= M2_NATURAL_PHASE2_CAP)` `:3937-3938` | the kickoff's taker can move | verify; a cap breach is a stop (a structural bound, not a measurement) |
| S6 | the first-bind identity/constant `M2_NAT_BAND_ENTITY = 12` `:1932-1938`, `bind_*` cells `:2585-2590` | the first bind may become a *moving* bind or another record | the constant re-pins only with a written reason (its own comment declares the sensitivity); the stderr records are re-based |
| S7 | the driven passes `:4473-4659` (claim) and `:4861-5041` (offset) | the slot records' walk speed rises too (their `timer7b` is the same produced cell), so every exact literal — `lane_x`, `bound`, `pos`, `face7d`, `cam_pos`, the stop place, `changed_frames`, the segment gates — is at risk | re-measured and re-pinned **in P1**, segment-aware, no staging (FU322's erratum 2 form) |
| S8 | the m-42/link early-warning fixtures `:2954-3080`, `:3198-3219`; the row-04 fixtures `tests/test_engine_match_frame.c:7535-7824` | the staged fixtures sit on records whose stride now moves | run **first**; a RED is a measured consequence to be re-pinned with the producer's chain named, never a silent tape move (FU321 §5.4 item 2) |
| S9 | the row-09 unit pin `test_action_09_stage0_set_branch`, `tests/test_engine_match_handlers.c:3613` (`target_x == pos_x`, the "target = pos" pin) | with the OL-82 correction the row-09 body stages `vel73/vel75` and the native `target = pos + vel<<3` (`0x80B91..0x80B9D`) makes the target leave the position — the one existing pin FU331's audit names as moved | re-pinned in P1 to the measured target, with the reason (the velocity is now live); the native add is not removed |

S7 is the surface FU332's brief does **not** name (Task 3 measures the golden
and the natural pan): it is a Stage-2 measurement obligation and the reason a
"P1 is just the producer" reading is wrong (FU333-L3).

### 4.7 Branch B (no tuple measured)

If FU332 returns NO-GO, P2 lands **no test change**: the measured-distribution
register (FU332's movement table + FU327's pre-producer control + the
reproduced phase-33 silent bind as the *pre-producer* baseline) and the legs,
with the A3/golden move still in P1. A *new* assert of the produced-but-zero
seed is deliberately not taken — it would pre-commit the acceptance to the
no-witness world and invert the day the witness appears (FU321-L11; FU329 §7.1).

---

## 5. The A3 re-pin

### 5.1 The current pin (fresh read)

```
tests/test_engine_m2.c:1948-1949   #define M2_NAT_FACE_FIRST_LO 4930
                                   #define M2_NAT_FACE_FIRST_HI 5200
tests/test_engine_m2.c:3977-3979   assert(nat->first_left_zero_step > nat->phase2_step);
                                   assert(nat->first_left_zero_step >= M2_NAT_FACE_FIRST_LO);
                                   assert(nat->first_left_zero_step <= M2_NAT_FACE_FIRST_HI);
```

The window's derivation comment (`:1939-1947`) states its own slack: ±135 steps
around the measured bind step 5065, justified by the **measured face-coupled
pacing scale** (the drives' gate/event cells 1512→1552 / 1513→1553, the claim
1514→1482), with the metric sub-change adding nothing (FU324's no-op). The
register carries the same figure (`docs/ENGINE.md:6812-6815`) and the phase-34
register hands the re-pin to this producer (`:7406-7407`).

### 5.2 The new anchor chain (the producer's own)

A3 pins the **first camera-cell departure** (`first_left_zero_step`,
`:3785-3795`: the first sample where any of the 16 cells is nonzero). Under the
producer the chain that precedes it is:

1. **the placement copies** — the produced limit reaches `+0x7B` at three sites:
   the row-install reset (`fifa96_match_entities.c:94`, native `0x7DA9B`), the
   kickoff placement (`:209`, native `0x79a86`), and record 0's keeper-machine
   copy (`:744`, native `0x7836A/0x78374`) — all
   `entity->timer7b = entity->timer79`. The limit itself is written in the
   match-start/kickoff/period-transition prelude (`FUN_0007997C` `0x79A4D`, then
   the `+0x7B` copy at `0x79A86`), with two further writers outside P1's scope
   (the lineup rebind's `+0x79>>1` floor 2, `0x8C939/0x8C942`, and the row-12
   no-replacement `max(+0x79>>1, 3)`, `0x822E7/0x822F0` — FU330 §2/§3), and
   P1's regression writes **both** cells at the init seam, so the value is live
   from the first MATCH frame (`match_run.c:2378`; FU331 §2);
2. **the first mover read** — `fifa96_match_run.c:549`
   (`s.stride = (uint8_t)(r->timer7b & 0x0Fu)`) → `fifa96_action_handlers.c:257-264`
   (`stride_len`, `attr`, the table) — the first frame a record's velocity word
   can exceed its pre-producer cap;
3. **the row-09 channel** — `fifa96_match_handlers.c:5034` writes
   `r->timer7b` (the port's staged 0 today; the function's own prologue says the
   native is `+0x7B = +0x79*2`, `:4962`, and that `word[+0x79]` is staged zero
   "OL-82", `:4967-4971`) and immediately feeds
   `vel73`/`vel75`/`speed71` (`:5036-5038`) and the target's `vel<<3` adds
   (`:5042-5043`). **P1 changes this site (OL-82), so the producer has two
   channels**;
4. **the first position move** — the mover's integrator
   (`fifa96_loader/fifa96_action_handlers.c:280-281`);
5. **the first camera departure** — the row-04 gate/event (`first_left_zero_step`);
6. **the first motion** — the event's `v0` run.

**The first-diff anchor for the golden is a *step*: `min(the first step at which
a mover reads a nibble > 1, the first step at which the row-09 body writes its
live value)`** — both measured (FU332's movement table; re-measured in P1). With
the port's produced value `6` (FU330 §2), the diagnostic's `+0x79 = 6` row —
engine step 2355 — is the closest expectation; the measurement is FU332's. This
is directly comparable to the golden's `frame=` column, which is the engine-step
clock too (the transcript's presents count, `:3430`, with one present per step,
`:3428-3429`) — one of the few places where the three-clock rule collapses. It
replaces the phase-33 anchor (9/52/852) for this producer: the faces-settle
chain is history, and 852 is *not* a valid bound here (the producer acts from
the first placement — FU331 writes both cells at the init seam, from frame 0 —
so the diff may start near the transcript's head). A diff **earlier** than the
measured minimum is a stop (TW1).

### 5.3 The rule

- **The values**: `LO/HI` = FU332's measured `first_left_zero_step` ∓ a
  **declared slack**. The slack covers the *alignment* class only — the
  one-frame gate-vs-capture offset between the event capture and the row-04
  body (FU328-L6), converted into steps at the measured pace — because the null
  tape is byte-deterministic (the two-run identity, `:5146-5150`) and has **no
  transient to cover** (FU322's erratum 3 precedent, re-based here). It is
  **not** the phase-33 ±135: that slack was the face-coupled walk's, and this
  producer's pacing is the stride's. The arithmetic is `LO = measured − slack`,
  `HI = measured + slack`; the numbers are FU332's and the slack is written at
  the definition site, where the current comment already puts its derivation
  (`:1939-1947`).
- **The exact step stays recorded** (the stderr line, `:6610-6618`), never
  pinned — the phase-33 form.
- **The first clause survives structurally**: `first_left_zero_step >
  phase2_step`. A departure *before* phase 2 is a stop-and-report (it would mean
  a pre-phase-2 camera event; the clause's whole point is "the natural event is
  a phase-2 consequence").
- **The identifiers stay** (`M2_NAT_FACE_FIRST_LO/HI`): the phase-33 register
  names them (`docs/ENGINE.md:6813`), its leg list records the values (`:7030`),
  and the phase-34 hand-off rules the re-pin to this producer (`:7406-7407`);
  the name is history, and the
  comment is re-based in P1 to "the first camera departure under the produced
  placement limit". A rename is cosmetic churn that re-cites the docs; not
  taken.
- **The commit**: P1 (§2, §6.4). P2 verifies; a moved A3 after P1 is a stop.

### 5.4 The natural-pass values that do *not* re-pin

`camera_zero_frames`, `camera_left_zero`, `first_left_zero_f`,
`first_left_zero_pos`, `first_bind_f/id` and the `nat_pan` cells are
**recorded** (stderr, `:6610-6664`), so their figures move without a re-pin;
the A2 identity `camera_zero_frames == first_left_zero_f` (`:3971`) survives by
construction (the all-zero prefix and its first non-zero sample move together),
and A1 `camera_left_zero >= 1` (`:3967`) survives. A2's *figure* must not be
re-quoted as 1517 in any new comment (that is the pre-producer baseline; the
register's phase-33 text stands, the phase-35 register quotes the new one).

---

## 6. The golden recipe (the faithful move)

### 6.1 The expectation, stated honestly

M2 **will** move, and probably wholesale: every record's stride rises and the
formation actually runs (FU328-L8). The move is P1's intended consequence, not
a regression — the difference from FU329's Case A is that this phase *owns* the
move. The recipe below is FU323 §6/FU329 §6.2's house form with this
producer's anchors.

### 6.2 The procedure

| step | command / artefact | expected shape |
|---|---|---|
| 1. the producer's own pins | unit pins: the reset write (`+0x79` → `+0x7B` at the install), the row-09 site's value, the counter copy; **RED on the pre-fix tree** | a value mismatch, not a compile error (the T1 two-step RED discipline, `docs/ENGINE.md:6858-6861`) |
| 2. the early-warning fixtures **first** | the m-42/link block (`:2954-3080`, `:3198-3219`), the row-04 fixtures (`test_engine_match_frame.c:7535-7824`), the row-09 unit pin (`test_engine_match_handlers.c:3613`), the **driven passes** (§4.6 S7) | a genuine RED, measured; never a silent tape move |
| 3. the candidate transcript | `./build/test_engine_m2 > /tmp/opencode/fu333/m2-new.txt` (ISO present, CWD = repo root) | — |
| 4. **the first-diff rule** | `cmp tests/golden/engine/m2-frames.txt …/m2-new.txt` → byte + line; the line's `frame=` (the engine step — the presents count, `:3428-3430`) must be **≥ the measured anchor step** (§5.2's minimum) | an earlier id = **STOP** (TW1) |
| 5. the `state=` check | `diff <(cut -d' ' -f3- golden) <(cut -d' ' -f3- new)`; `grep -c ' state=' golden new` (6 517 today, fresh) | empty / equal; a moved score-bearing suffix or a new tag = **STOP** (TW2) |
| 6. the stratum-set gate | in-test `:7021-7089` (the phase-33 **Outcome B** gate, 10 registered tags; a NEW tag fails the lookup, a LOST tag fails `distinct == nreg`) | no tag added or lost; the per-tag counts **recorded** (the phase-33 counts `state=1` ×951 / `state=19` ×2 620 / `state=2` ×2 100 are history, not a bound) |
| 7. the line-count invariant | `wc -l` (6 522 today) | unchanged; a moved line count = **STOP** — the transcript ends on the ET-terminal directive chain (`:3690-3695`), not on open play, and the phase-33 re-pin kept 6 522 |
| 8. M1 immutable | `./build/test_engine_m1 \| cmp - tests/golden/engine/m1-frames.txt` | silent; a move is a regression report, never an upgrade (AGENTS.md) |
| 9. the written reason | the house form (`tests/test_engine_m2.c:1716-1750`): old → new sha (both 64-hex), bytes/lines, the first/last differing line + `frame=`, the contiguous range, the moved columns, the strata + counts, the anchors crossed (m 41 / m 42 / m 58 / the v21 2 522-line prefix — crossing it is expected now, and the stratum gate is already in place), the intended consequence (the anchor chain of §5.2, with both channels' measured first frames), the collateral classified (the walk-in/AI-chase class + the row-09 velocity class), and the **FU324-L5 re-measure clause** (§8/H10) | complete |
| 10. the re-pin + the register | `./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`; the register pin in `docs/ENGINE.md` + `docs/PROGRESS.md` in the same commit; `make check` green in the same commit | one commit |

### 6.3 The tripwires (all stop-and-report, spec §7)

- **TW1** the first diff is earlier than the measured anchor (§5.2's minimum).
- **TW2** a `state=` suffix's value moved (the score line) or a tag appeared/
  disappeared — the registered set is not re-registered silently (§8/H8).
- **TW3** the line count ≠ 6 522 (the scenario shape changed).
- **TW4** M1 moved.
- **TW5** a moved frame neither channel can explain (classification against the
  measured first-frames list, not against prose).
- **TW6** the m-42/link, the row-04 fixtures or a driven pass went RED without a
  measured producer cause.
- **TW7** the golden re-pin rode P2, or A3 was re-pinned in P2 while P1 was red.
- **TW8** P1 changed a `+0x7B` writer other than the produced chain (H6), or the
  natural pass gained a staged cell (§4.4).
- **TW9** `make check` is not green at a commit, or the census/README moved
  without a fresh dispatch count (H9).

### 6.4 The P1/P2 commit split

| | P1 — the producer (the atomic commit) | P2 — the acceptance (test-side only) |
|---|---|---|
| content | the producer (FU331's shape) + the row-09 OL-82 site + the unit pins + the natural-pass survival re-pins (S1–S6) + the driven-pass re-pins (S7) + the m-42/link re-values if measured (S8) + the row-09 unit pin (S9) + the golden re-pin + the provenance note + the FU324-L5 re-measure + the register/PROGRESS pin | the natural witness pins (§4): the ring, the chain cells, the per-segment record, the literal tuple, the no-staging checks — **or** the measured-distribution register (Branch B) |
| golden | re-pinned here, once, with the full accounting | **cannot** move it (the natural pass writes no transcript, `:3859-3864`, `:5146-5163`) |
| A3 | re-pinned here (§5) | verified, never re-pinned |
| commit type | `fix(fu328)` (the leg it closes; the phase-33 `fix(fu314)` precedent) | `test(fu328)` / `test(fu329)` |
| green gate | `make check` green in the commit | `make check` green in the commit |

The separation is the spec §8.1 rule ("its own commit, its own RED/GREEN, its
own golden accounting") and the FU329 erratum 4 rule ("never folded into the
acceptance").

### 6.5 The pins and the invariants (fresh this session)

| golden | sha256 | bytes | lines |
|---|---|---|---|
| M1 `tests/golden/engine/m1-frames.txt` | `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` | 22 004 | 691 |
| M2 `tests/golden/engine/m2-frames.txt` | `d5446144f08c22c5a4d986e1d07529068f715ff8b8e2cd86411ee493ea804ad8` | 295 087 | 6 522 |

`grep -c ' state=' m2-frames.txt` = **6 517** (5 pre-match lines carry no tag);
the 10-tag registered set is `:7033-7038`. Census **`60/80 = 75%`** (`37 action
+ 23 phase`; `docs/ENGINE.md:8`, `:268`; README `:1`); the producer wires **no**
dispatch row, so the count and the derived README title must not move
(AGENTS.md). `make check` is recorded as 111/111 by the phase-33/34 registers;
Stage 2 records the count of its own tree.

---

## 7. The Layer-2 live design (if the witness lands)

The vehicle is the **undriven live run** — the phase-33 drive is not used:
`DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/fu333/trace.log make game`; the
walkthrough intro RETURN → front-end BACKSPACE → RETURN; then the `M2_KEYS`
prefix replayed by **trace tick** with **nothing added** post-phase-2 (spec §6;
FU329 §5.1). Global XTEST only (the `--window` XSendEvent route leaves the
act-1 KICK's release unobserved, `docs/screens/p33-l2-notes.md` timing limit 1);
no lossless encoder during the walkthrough.

- **W-a (the wall-clock waiver)** stands by construction: no post-phase-2 drive
  exists, so FU321-L4's scheduling rule has nothing to schedule; the prefix is
  trace-tick-driven and every anchor comes from this run's own trace.
- **W-b (the pair width)**: the pan is 6/13/20/27/27 granted frames for
  `|v0| = 1..5` (`m2_pan_curve`), ≈0.2 s at `|v0| = 1` (FU329 §5.3) and
  proportionally ≈0.9 s at the top — measurable from FU332's tuple, not assumed.
  If a same-second pair fails, the p31 clip fallback, then the waiver; the
  waiver weakens only the AE, never the trace.
- **The claims**: trace primary (the camera cells only,
  `fifa96_match_run.c:4657-4671`), the AE relative (`AE_pan > AE_still`,
  same session/build/run), the still control as a camera-constant trace pair
  that may be non-pixel-identical (the p33 ball residual precedent), the
  whole-scene bbox as the sprite-only caveat (never an attribution), each pair
  anchored to **one named segment**.
- **The verdict wording**: "a natural no-slot pan reproduced live under the
  undriven tape" — never "the trace shows the seed" (Layer 1 owns the chain) and
  never "the natural route pans" without the measured tuple. The slot-arm drive
  is not a fallback (FU329-L5).
- **The artefact set**: `docs/screens/p35-l2-notes.md` + the pairs/diffs + the
  trace extract, in the p31/p32/p33 form.

---

## 8. The hazards

| # | hazard | authority | tripwire / rule |
|---|---|---|---|
| **H1** | **The row-09 interplay.** `fifa96_match_handlers.c:5034` writes `r->timer7b = 0` on the row-09 walk; the same function's prologue (`:4962`, `:4967-4971`) says the native is `+0x7B = +0x79*2` and that `word[+0x79]` is staged zero "OL-82". So a produced limit turns the staged-zero velocity staging into a **second movement channel**: `vel73/vel75` (`:5036-5037`), `speed71` (`:5038`), the target's `vel<<3` adds (`:5042-5043`). | fresh read; FU331 (the design); spec §8.4 | P1 retires or corrects OL-82 **in the same commit** (spec §6), and the golden's first-diff anchor is the earlier of the two channels (§5.2). A witness whose chain cells contradict the nibble is a STOP. |
| **H2** | **The OL-82 retirement.** The comment/assumption `+0x79 zero` is a *value* claim, not just a note; "retired" without a measured value change would be a prose edit over a live bug, and "corrected" without the native form would invent one. | `:5034`, `:4962`, `:4967-4971`; FU178/FU84 §6 | the corrected expression is FU331's, from FU330's census; P1's unit pin must state the value before/after; no other `+0x7B` writer changes (H6). |
| **H3** | **The whole-formation AI movement.** The install reset raises every record's nibble → the formation's speed, the chase/defender decisions, the code-4 population (`docs/ENGINE.md:6967-6979`, the phase-33 collapse class in reverse) and possibly a **goal**. The natural pass asserts a scoreless window at every sample (`:3910-3913`) and the run's shape depends on it. | fresh read; FU323-L4; FU332 (the measurement) | the diff's collateral is classified as this class; a score change is TW2 (a hard stop); the T3 negative (S4) is a stop-and-report, never a loosened assert. |
| **H4** | **The `+0x9D` OR (OL-72).** The native store is `byte[0x10F394 + (byte[[rec+4]+0xD] sign-extended)] \| [rec+0x9D]` (FU330 §2/§3; FU84 §6 carries the table, its `+0xA` index byte superseded). `+0x9D` is recomputed by the lineup rebind (`0x8C7A6..0x8C82D`, values 0 or `0x0F`) and is **unported**; the pool is BSS zero at kickoff (FU330 §2), so the port's OR cell is 0 *now* but not necessarily later. | FU330 §2/§3; FU328 erratum 2; OL-72 | the port's produced value is the table byte (`6` while `player_d` is staged 0); if a later rebind would set `+0x9D = 0x0F` (FU330's OR producer), the port under-values it — a leg, never a byte invented for it. |
| **H5** | **The no-slot / `lane = 0` / zero-seed traps (carried).** A pass at the camera (`lane_x 0`) is the zero-offset fold; a `v0 = 0` bind or a zero place delta is not a witness; a staged slot/seed/face cell disqualifies it. Plus the new trap: the row-03 idle core writes `min((0>>22)+1, timer79) = 1` into `+0x7B` (`:5813-5822`), which **clamps** the produced limit to 1 — it cannot enable a `\|w\| ≥ 3` seed, so a witness whose nibble is 1 with a large seed is contradictory. | spec §7; FU329 H1/H2/H3; FU328 §3.4 | `lane_x ≥ 1` asserted; the chain bound asserted; the `lane = 0` conflict is FU329-L2's ruling (not a fixture decision). |
| **H6** | **The other `+0x7B` writers are *not* the producer.** The port stages constants for several native `+0x7B` writes: the phase-0C `= 2` (`:5846`; native `table>>1`), the row-18 duel `= d.stride` (`:2344`; native `= 2`), the row-0x19 human-side `= 1` (`:4861`; native `0x74CA8`), the row-26 loader `table[player_d] >> 1` (`fifa96_arm_bodies.c:51-52`, reading the **same** `0x10F394` table the port already carries at `:29-37`), `:449` `= 2`, `:594` `= 4`, `fifa96_keeper.c:502` `= 3`, `:562` `= 2`; plus the slot-branch copy `timer7b = timer79` (`:5785`) and the pool repacks (`:497/737/749/848/866`). | fresh read (`arm_bodies.c:29-37` names the table flat `0x10F394`); FU330 §3 (the index byte `+0xD` is shared with row 26 — the difference is row 26's `player_d 0..31` guard and the `>>1`, **not** the index); FU331 §3 | **one channel per commit**: P1 changes the produced limit chain (the init/reset writers + the row-09 site FU331 rules); any other value change is a separately measured, separately pinned commit. A copy/paste of the row-26 read is a specific trap: the guard and the `>>1` (the index byte is the producer's own). |
| **H7** | **The survival-pin class.** A3 is not the only pinned value: `granted_frames == 3598`, `stage.calls == 6118`, `kick_step ≤ 600`, the first-bind identity, and the driven passes' dozens of literals all move or must be re-valued. | §4.6 | re-valued **once**, in P1, from the measured build; a moved grant count or a cap breach is a stop. |
| **H8** | **The golden's strata and the score.** The registered 10-tag set (`:7033-7038`) and the per-line `state=` suffixes encode the phase progress *and the score* (`state=2/1-0` vs `state=2/0-0`). A producer that lets the formation score changes a tag's suffix — a new and a lost stratum in one move. | `:7021-7089`; FU322's erratum 5 | a suffix *value* move is TW2 (stop); the set is never re-registered silently; the phase-33 counts are recorded history. |
| **H9** | **The census/README invariants.** `60/80 = 75%`; the README title is derived, never hand-tuned; **the row-03 placement-counter wiring is not part of this producer** — FU328-L2/FU177-L1 is a separate leg, and any dispatch-table change needs its own fresh count and the README recompute in the same commit. | AGENTS.md; `docs/ENGINE.md:8`, `:268`; README `:1`; FU328-L2 | grep the census before/after; a wired-count change without a fresh census = NO-GO; a README recompute without one is forbidden. |
| **H10** | **The FU324-L5 re-measure class.** The `+0x7B` nibble staging's arrival is the named trigger for the mutant protocol (FU324 §5): the `stride_len ≤ 3` clamp that masked every metric difference is gone, so the mover's metric sensitivity returns. | `docs/ENGINE.md:7073-7077`, `:7530-7531`; FU324-L5 | P1 re-runs the mutant protocol and records the result in the register; it is a **clause**, not a silent re-measure. |
| **H11** | **The reading traps (carried).** `ev` is the event *height*; `pan` counts pan *steps*; the off-zero count is not the run count; the sampler is post-frame while the mover reads mid-frame (`+0x7B` may be rewritten between the two); `f`/`step`/`tick` are three clocks; the diagnostic is not a witness. | FU321 §4.3/§4.6; FU317 §3.4; FU328 §5.6 | the extract names its run and its clocks; the chain consistency (W3) is recorded and investigated on a mismatch, never "explained"; the diagnostic stays out of the tree. |

---

## 9. Numbered legs

1. **FU333-L1 — the tuple is FU332's.** This doc fixes the slots (W1–W7), the
   classes and the fail signatures; every literal (frames, record id, nibble,
   seed, event pair, place, pan) is FU332's measured tuple, re-run once. Owner:
   FU332 → the Stage-2 P2.
2. **FU333-L2 — the A3 commit placement (spec §6 vs the green gate).** Spec §6
   says "re-pins in P2"; a P1 that re-pins M2 while A3 is red is not a valid
   commit (AGENTS.md; spec §8.1), so the re-pin lands in P1 and P2 verifies.
   The spec sentence needs a Stage-2 erratum; until then this is the ruling and
   the contradiction is recorded, not hidden. Owner: the Stage-2 plan /
   controller.
3. **FU333-L3 — the driven passes are a second acceptance surface.** The claim
   (`:4473-4659`) and offset (`:4861-5041`) passes pin exact literals on records
   whose stride the producer raises; FU332's brief does not measure them. P1
   must re-measure and re-pin them (segment-aware) or the commit is red. Owner:
   P1.
4. **FU333-L4 — the natural-pass value pins outside A3.** `granted_frames ==
   3598`, `stage.calls == 6118`, `select_ran == 0`, the kickoff cap, the
   first-bind identity: re-valued once in P1; a moved grant count is a stop.
   Owner: P1.
5. **FU333-L5 — the score-bearing strata.** A change of `state=*` suffix value
   (the score) is a hard stop; the registered set is not re-registered without a
   written ruling. If the measured producer genuinely scores on the tape, the
   phase re-scopes the natural window — it does not loosen the gate. Owner: the
   Stage-2 + the wave review.
6. **FU333-L6 — the two producer channels.** The placement copies and the row-09
   site (OL-82) are both P1's; the golden's first-diff anchor is the earlier of
   their measured first *steps*. Owner: P1 (measure), FU333 (rule).
7. **FU333-L7 — the `+0x9D` OR's port value is 0.** OL-72 is unported, so the
   port's produced limit is the table byte; if FU330 finds a nonzero `+0x9D` for
   the band record, the port under-values it and the acceptance records the
   divergence (a value divergence, not a mechanism divergence). Owner: FU330 +
   P1.
8. **FU333-L8 — the `+0x7B` writer census is one channel per commit.** The
   phase-0C/row-18/row-0x19/row-26/keeper stagings (H6) are not the producer;
   changing any of them in P1 is TW8. Owner: P1.
9. **FU333-L9 — the row-03 idle core clamps the produced limit to 1.** It is
   not the witness's enabler (the placement copies are); a witness whose nibble is 1
   with a large seed is contradictory, and the enabler's identity is recorded in
   the commit. Owner: P1/P2.
10. **FU333-L10 — the row-26 read is the same table and the same index byte,
    not the producer.** The port already carries the `0x10F394` bytes
    (`fifa96_arm_bodies.c:29-37`) and row 26 reads them with `[+D]` and a `>>1`
    (`:51-52`) — the **same** descriptor byte FU330's producer indexes, with a
    `player_d 0..31` guard the producer does not have. The trap is the guard and
    the shift, not the index. Owner: FU331's audit (its §3 classifies the seven
    loader/arm sites outside the producer's blast radius).
11. **FU333-L11 — the FU324-L5 re-measure rides P1.** The nibble staging's
    arrival triggers the mutant protocol; the result lands in the register as a
    clause, not a silent re-measure. Owner: P1.
12. **FU333-L12 — Layer 2 is conditional on the tuple and on the measured pan
    duration.** W-a/W-b as FU329; a failed live natural run is
    NOT-GO-as-measured with the Layer-1 witness standing; the slot-arm drive is
    not a substitute. Owner: the Layer-2 task.
13. **FU333-L13 — the armer/OOB accumulation of a natural pan is recorded, not
    asserted.** `T ≤ 0x90` per axis (FU326 §4.3) bounds one pan, not a chained
    place; the acceptance records the cumulative place and asserts only the
    witness's own accumulation (FU329 H5/L6, carried). Owner: the acceptance
    wording.
14. **FU333-L14 — the suite count is per-tree.** `make check` is 111/111 today;
    Stage 2 records its own tree's count and does not inherit the number.
15. **FU333-L15 — `+0x79`'s other writers are outside P1.** The lineup rebind
    halves the limit (`+0x79>>1` floor 2, `0x8C939/0x8C942`, iff `+0x99`) and
    the row-12 no-replacement arm writes `max(+0x79>>1, 3)`
    (`0x822E7/0x822F0`) — both unported (FU330 §2/§3). P1 lands the placement
    writer only; an acceptance whose window crosses one of these re-writes is
    **not** covered by the pinned tuple and must be re-measured, never
    explained by the placement value. Owner: the Stage-2 (the writers' own
    phase).

---

## 10. GO / NO-GO

**GO — the acceptance design, both branches.** Every clause above is
implementable from the frozen authority plus FU330–FU332; the pin surfaces are
first-hand reads; the golden recipe is the phase-33 house form with this
producer's anchors; no number is invented here.

**The Layer-1 GO is conditional on FU332's measured tuple** and on these
conditions:

- **(C1) the arm** — `code == 4`, `anim_id != 0x13`, `cam_pos_y == 0`,
  `has_slot 0`, the `0x7F056` camera sub-gate, `tracked != id`, `controlled !=
  id`, and the seed `== (int16)(vel73 << 2)` (not `dir << 6`);
- **(C2) the chain** — `timer79` live at the gate, the `timer7b & 0x0F` nibble
  at FU332's measured value, `|vel73|,|vel75| ≤ nibble`, the install-reset
  channel named;
- **(C3) the gate** — `lane_x ≥ 1`, the three lane terms, Δ ≤ `0xAB` on the live
  face;
- **(C4) the event** — `|v0| ≥ 1`, the fast-path identity (or the measured pair
  above the `0x19` cap), `trk == id`, `tim 0/12`, `ramp 12`;
- **(C5) the run** — `segments ≥ 1`, `changed_frames`/stop/place/pan from
  `m2_pan_curve`, `pos_y == 0`, the one-shot;
- **(C6) the honesty** — no staged cell (§4.4), the tuple recorded not derived,
  the frequency recorded, `lane = 0` → NO-GO.

**The golden GO is P1's faithful move** (not FU329's Case A): it is expected,
large, and governed by §6 — the accounting, the anchors, the strata, M1, the
written reason, `make check` green in the same commit. **P2 is byte-identical by
construction.**

**NO-GO on:** (a) a witness at `lane = 0`, `v0 = 0`, or a zero place delta;
(b) any staged slot/seed/face/timer/camera cell in the natural witness;
(c) the diagnostic ladder as a witness; (d) a golden re-pin without the frame
diff, the `state=` check, the stratum-set gate, M1 and the written reason in the
same commit; (e) an A3 re-pin in P2 while P1 is red; (f) a second `+0x7B` writer
changed in P1; (g) a score-stratum re-registration without a written ruling;
(h) wiring the row-03 counter (the census move) into this producer; (i) an
attribution from the AE or the bbox alone; (j) a witness measured on a tree
whose producer differs from FU330's census (a guessed value is not a producer);
(k) a witness whose window crosses a `+0x79` re-write P1 does not model (L15).

---

## 11. Known limits

1. **No measurement in this slice** (deliberately): the tuple, the pacing and
   the blast radius are FU332's; the native values are FU330's; the producer's
   shape is FU331's. Every number here is a frozen pin or a forward-cite.
2. **The A3 values are not written**, only their rule (the measured step ∓ the
   declared alignment slack): the slack covers the capture/body one-frame offset
   (FU328-L6) at the measured pace, and FU332's re-run once plus the null tape's
   two-run identity (`:5146-5150`) are its evidence that no transient exists.
3. **The witness pin is designed, not written**: the ring, the chain cells and
   the per-segment record are test-side work whose exact line placements move
   when they land (the phase-33 final-review erratum is the precedent for fresh
   re-cites).
4. **The first-diff anchor depends on a measurement P1 must re-take** (the two
   channels' first frames); TW1 is only as good as that number.
5. **The score-stratum risk (H8) is unquantified here** — whether the produced
   movement changes the golden's score is FU332's diff, and a change is a stop,
   not a re-pin.
6. **The live rate is unmeasured** (the null tape is input-starved and
   deterministic; FU328-L5 carries).
7. **`+0x9D`'s post-rebind value is unmodelled and the native per-record `+0x79`
   is unmeasured.** The pool is BSS zero at kickoff (FU330 §2, fresh read), so
   the port's OR cell is 0 *there*; the lineup rebind's `+0x9D = 0x0F` branch is
   unported, and the native per-record values stay FU330-L1/FU328-L3 (the
   census narrows the *table* domain, not the roster data).

---

## 12. Provenance and self-audit

**Fresh this session (repo reads only; no writes to code, no git, no Ghidra, no
probe, no build):** `AGENTS.md`; the spec §1–§9; the plan `:1-116`;
`docs/ghidra/FU329_r2l8_acceptance.md` (whole incl. its errata),
`FU328_r2l8_reachability.md` (whole incl. its errata),
`FU323_r2l7_m2_probe.md` §6–§10, `FU322_r2l7_baseline_inversion.md` §6–§8 +
errata, `FU84_animation.md` §6; `docs/ENGINE.md` (`:1-12`, `:260-275`,
`:6700-6750`, `:6775-6800`, `:6858-6861`, `:6960-6980`, `:7028-7080`,
`:7184-7260`, `:7398-7420`, `:7453-7540`); `tests/test_engine_m2.c`
(`:1716-1750`, `:1880-1999`, `:2423-2540`, `:2540-2660`, `:3746-3870`,
`:3900-3979`, `:4003-4013`, `:4219-4290`, `:4473-4572`, `:5130-5164`,
`:6610-6664`, `:6970-7105`); `src/fifa96_engine/fifa96_match_entities.c`
(`:80-100`, `:200-215`, `:735-750`), `fifa96_match_run.c` (`:490-564`),
`fifa96_match_handlers.c` (`:2325-2352`, `:4805-4870`, `:4955-5052`,
`:5770-5850`); `src/fifa96_loader/fifa96_action_handlers.c` (`:235-284`);
`src/fifa96_loader/fifa96_arm_bodies.c` (`:32-69`); the goldens
(`sha256sum`/`wc`/`grep -c`, §6.5); README `:1`.

**Carried (owner docs named, never re-derived):** the producer form — FU330 §2/§3
(the fresh census: the `0x10F394` table, the `+0xD` roster index, the OR, the
three writers, the prelude timing, the port's produced 6) and FU84 §6 (the
earlier carried form, its `+0xA` index byte superseded by FU330); the `+0x79`
gap and the diagnostic ladder (FU328 §5.4/§5.6/L1–L3, erratum 2); the port's
producer design and the OL-82 correction (FU331 §2/§3); the seed domain and the
silent band (FU326 §4.2/§4.4); the five measured passes and the pre-producer
`max |w| = 1` (FU327 §2/§4); the Branch B tuple/split/recipe/hazards (FU329
§4–§8); the 9/52/852 chain and the faithful-move form (FU323 §6; FU322's
erratum 5; `docs/ENGINE.md:6775-6797`); the A3 window and the hand-off
(`docs/ENGINE.md:6812-6815`, `:7406-7407`).

**Self-audit.** (a) Every port/test claim is a fresh `file:line` read; every
native statement is a named carried doc; FU330's and FU331's qualitative census
findings are cited as owners (including the port-derived value 6, which is a
*port* value), and every quantity **FU332** owns — the measured blast radius,
the tuple, the pacing — is a forward-cite written as *the measured value*; no
measured number of FU332's is asserted here. (b) The only arithmetic statements
are the port's own forms quoted with
their lines (`timer7b = timer79` at `entities.c:94`; `stride = timer7b & 0x0F`
at `run.c:549`; `stride_len`/`attr` at `action_handlers.c:257-264`;
`s = w << 2` at `fifa96_outfield.c` per FU328 §3.2; the curve
`tests/test_engine_m2.c:2467-2481`), each labelled a form, never an achieved
value. (c) The spec §6 "A3 in P2" is contradicted by the green gate and the
contradiction is a numbered leg (FU333-L2), not silently rewritten. (d) The
unproven items are legs L1–L15. (e) The parallel drafts **FU330** and **FU331**
landed while this draft was being written and were read at their landing; their
qualitative findings are cited as owners (the index byte `+0xD`, the three
`+0x79` writers, the port's produced value 6, the OL-82 correction, the S9 unit
pin), and FU84 §6's `+0xA` index byte is corrected by FU330 (a wave-review
erratum, not a rewrite). **FU332** had not landed when this draft closed: every
measured number it owns remains a forward-cite (L1). (f) This doc creates one
repo file, runs no git command, writes nothing to `build/`, `tests/golden/` or
the ISO, and runs no probe; the only scratch path named
(`/tmp/opencode/fu333/`) is unused.

## Review erratum (wave review, 2026-10-23)

**1. "Branch B" is the no-witness branch.** Quote (§2): "The witness is FU329's Branch B concretized on the **undriven `M2_KEYS` tape**"; quote (§4 heading): "## 4. The Layer-1 acceptance (Branch B concretized)". **Corrected (FU329 §2.1/§7):** FU329's **Branch A** is the witness measured; **Branch B** is the no-witness / measured-distribution branch. FU333's own §2.1 table also has Branch A = FU332 measures the tuple and Branch B = it does not. The witness design is **FU329's Branch A** (FU333's Branch A); §4 must be retitled, and §4.7's no-tuple branch is the only Branch B. As written, a Stage-2 reader following "Branch B" would land no test change.

**2. The first-diff anchor is 852, not 2355/12.** Quote (§5.2): "The first-diff anchor for the golden is a *step*: min(the first step at which a mover reads a nibble > 1, the first step at which the row-09 body writes its live value)" … "the diagnostic's `+0x79 = 6` row — engine step 2355 — is the closest expectation" … "852 is *not* a valid bound here". **Corrected (FU332 §2/§5):** the producer's first pool divergence is **step 12**, the band **event** (the A3 camera departure) is **step 2355**, and the golden's first differing line is **frame 852** (`964282c7…` → `e04df34d…`, one contiguous tail 852..6522). 2355 is the A3/event anchor, not the golden's first diff; 852 is the measured first hash-visible frame. The golden tripwire's lower bound must be **852** (a diff in 13..851 is a stop — the render is byte-identical until the sit-1 reveal, FU323 §6), while the producer-attribution window starts at 12. "852 is not a valid bound" should read "852 is not a derivation from the phase-33 face chain; it is the measured render bound for this build."

**3. The witness is producer-conditioned.** Quote (§10): "The Layer-1 GO is conditional on FU332's measured tuple" (C1–C6 carry no qualifier). **Corrected (FU332 §2/§9):** the measured tuple (event f 704, entity 12, `has_slot 0`, lane 56, `vel73 −6`, seed `−0x18`, `v0 −2`, `T −0x26`, 13 frames, pan 2 at 6; event 607, `v0 −4`, `T −0x84`, 27, pan 4 at 15) is **producer-conditioned**, not the native witness while FU330-L1/FU328-L3 is open. C2 and the GO wording must say "producer-conditioned" and must not sell the golden as "the native golden" until the roster value lands. (FU332's narrow NO-GO is the right split; FU333 does not carry it.)

**4. "row-12" is action code `0x0C`.** Quote (§5.2 / L15): "the row-12 no-replacement `max(+0x79>>1, 3)`, `0x822E7/0x822F0`". **Corrected (FU330 §2/§3.3; fresh `0x1106E0[12] = 0x81C90`):** this is action code **0x0C** (the contact/no-replacement body), not row 0x12. Rename it in §5.2 and L15.

**5. The phase-0C hazard cite.** Quote (H6): "the phase-0C `= 2` (`:5846`; native `table>>1`)". **Corrected:** `:5846` is the **row-03 phase-0xC tail** (`0x7E510`), a constant 2; the phase-0C **row's** `table>>1` (`0x6DF78`) is a different, **unported** site (the phase-0C body's own comment at `handlers.c:6479` names the table as FU192-L1). The "one channel per commit" rule stands; the two sites must not be conflated.

**6. The spec's A3 sentence is in both §5 and §6.** Quote (L2): "Spec §6 says 're-pins in P2'". **Corrected:** the spec §5's P2 sketch also says "the A3 re-pin" ("P2 the natural no-slot pan acceptance (Branch B + the A3 re-pin; test-side only)"), so the controller's Stage-2 erratum must correct **§5 and §6**. The green-gate logic (P1, not P2) is sound and stands.

**7. The memcmp rationale.** Quote (§4.4): "the null platform consumes the copy, so a mutation is detectable". **Corrected (fresh `platform_null.c` `n_poll`):** the null platform only advances `tape_pos`; it never writes the tape. The post-run `memcmp(tape, M2_KEYS, …)` is a valid source-level check because any mutation is the pass's, not because the platform consumes it.

**8. "Probably wholesale" is measured.** Quote (§6.1): "M2 **will** move, and probably wholesale". **Corrected (FU332 §5):** the measured diff is **5 671 / 6 522 lines (86.95 %)**, one contiguous tail **852..6522**, the 10-tag stratum set intact, length unchanged. "Probably wholesale" should be the measured shape; the 851 pre-diff lines are unchanged.
