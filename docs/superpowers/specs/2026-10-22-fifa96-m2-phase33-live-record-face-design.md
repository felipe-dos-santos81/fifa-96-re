# FIFA 96 M2 phase-33 design — the live record face (the `+0x43` latch staged live)

## 1. Goal

Stage the native `+0x43` record latch (the locomotion turn's "steer the facing"
bit) and the camera-focus point live in the port, making the locomotion turn a
running producer instead of a staged-zero stub. This closes **FU314-L5** and
lifts the phase-32 deferral of **FU319-L5/L3**: after this phase the undriven
natural tape pans (the band record arrives facing its focus point, the row-04
angle gate passes natively, arm B arms), the phase-32 witness stops being a
plateau-luck artefact and becomes the robust route (FU314-L1a), and the
phase-30 NO-GO baseline inverts by design (FU321-L11).

## 2. What phase 32 left (the binding starting state)

- The phase-32 verdict named **the live face producer** the natural route beyond
  the plateau-riding witness drive, and deferred it on the spec §8.1 condition:
  a universal producer flips `38/111` rows' worth of M2/AI state and
  **FU319-L7's actual M2 diff was unmeasured** (the primary condition; see the
  code-review errata, `docs/ENGINE.md` phase-32 close-out errata item 4).
- The producer itself is **frozen authority**: FU319 §2 (the census: three
  writers, two readers, no other site), §5 (the minimal faithful staging),
  §8 (GO on staging the live `+0x43` and the focus point).
- The phase-30 natural-pass baseline asserts stand at
  `tests/test_engine_m2.c:3701-3707` with the site's own note
  (`:3720`: "the bootstrap producer will flip it and the P7/P8
  window/lockout assertions land with it"); P6–P8 are RED by design today
  (`:2293`, the honest-baseline label `:6049-6054`).
- Pins: M1 `09b726b7…` immutable; M2 `8434cfab…` is the re-pin baseline under
  the faithful-move policy; census `60/80 = 75%`.

## 3. Scope

**In slice:**
- The `row43` latch cell (native `byte[rec+0x43]`, values `{0,2}`), with the
  three native writers (the two anim-row installs `0x6E6C4`/`0x8DFBA`-family and
  the kickoff placement's narrow install `0x79C13`) and the one clear
  (`fifa96_match_action_22` stage 0, the `0x8540C` site).
- The mover staging: `s.direct_face = r->row43` and
  `s.face_x/face_z = (int16_t)camera.pos_x/pos_z` (the default focus pointer
  `[0x157A73] → 0x15774C/0x157754`), in `match_run_record_mover`.
- The **move_attr metric sub-change** (`0x8DCD4 → 0x8DC68` for the turn's
  `+0x63 >> 16` threshold and the `>> 19` stride attr) — in scope but **only as
  a separately pinned sub-change** with its own RED/GREEN and measured blast
  radius (FU319-L3: measured max |Δ| = 115 units, 2 branch-straddling
  record-frames; the stride index moves for every record).
- The natural-pass baseline inversion (FU321-L11) with the P7/P8
  window/lockout assertions landing **in the same commit** as the inversion.
- The M2 re-pin under the faithful-move policy (FU321 §5), including the
  re-check of the phase-32 driven/claim/offset witnesses under the live face.

**Out slice (stays legs, never asserted):**
- FU319-L1 (the non-default focus pointers — foul spot/restore, phase setters),
  FU319-L2 (the post-`+0x28 = 0` reset latch state, not expressible in the port
  yet), FU319-L4 (the frame-wrap install's one-frame placement), FU319-L5 (the
  sibling flag cells `+0x45`/`+0x3F`), FU314-L5a (the row-0x24 `(1,0)` face
  vector shape), FU314-L4's fallback semantics beyond what the wave measures,
  and the camera armer/follow work (FU316-L8/L9, FU315-L1/L2).
- No new native port, no new engine-half port. `row44` and the existing
  `anim_flags` decomposition stay as they are.

## 4. Native authority (carried, not re-derived)

- The turn: `0x8E29C CMP byte[+0x43],0` reads the live record byte; the turn
  branches on `move_attr >> 16 > 0x60` (the target branch) vs the focus branch,
  rotating `+0x7D` and `+0x8E` at `≤ (0x10 − speed) << (slot ? 4 : 2)` per frame
  (FU319 §3/§2.2; the port twin `fifa96_action_handlers.c:202-300`).
- The marketers of the change: `+0x7D`/`+0x8E` readers are the row-04 gate
  `0x7EE5C`, the defender decision, the row-0x0B clamp, the row-0x25 fold, the
  row-0x1B/0x16/0x24 face reads (FU319 §5 "what it changes").
- The latch producers and clear are singular (FU319 §2 census, §5, §8).
- The kickoff placement installs row 0 (or 0x26 behind the states) with the
  flags `0x07`, so **the native value is `2` for every record after kickoff** —
  the port's staged `0` suppresses a live, universal behaviour.

## 5. Structure

Two stages, the phase-32 shape:

- **Stage 1 (recon-ahead wave):** four read-only slices → FU322–FU325, one
  agent each, drafts under `docs/ghidra/`, whole-wave review + quoted errata,
  promoted one-per-commit. Probes are out-of-tree (`/tmp/opencode/`); the repo
  receives only the four draft docs.
- **Stage 2 (SDD execution, after the wave review):** the producer plan then
  implements. Sketch (the wave may re-shape): **P1** the latch + staging
  implementation with unit pins (RED→GREEN per writer/clear/turn arm, the M2
  probe measurement), **P2** the natural acceptance commit (the inversion +
  P7/P8 + the golden re-pin), **P3** the move_attr metric sub-change (own pins),
  **P4** the close-out register.

## 6. Acceptance (conditional on the wave's measured numbers)

- **Layer-1 (sim):** on the **undriven** natural tape, the band record's face
  settles facing the focus/camera and the row-04 gate passes band frames;
  `test_engine_m2.c`'s natural pass shows the camera leaving `(0,0,0)` from the
  first face-driven event onward. The probe-predicted numbers (FU319 §6: face
  `0x300` at the band, `2 082` band frames, the pan from the first band frame)
  are **re-measured by the wave probe** (FU323) before the Stage-2 plan binds
  them.
- **Unit pins:** the three writers, the one clear, the mover staging, and the
  locomotion turn's `direct_face` arm each get a RED→GREEN pin (the turn
  function itself is already pinned; only the staging is new).
- **Layer-2 (live):** the natural (+driven, if the wave's own prove needs it)
  game run shows the pan with the usual trace golden, AE-relative, sprite-only
  bbox caveat, and **trace-primary** rule.
- **Golden governance:** M2 re-pins only with the written reason in the house
  form (the first differing frame N, the number of moved hash lines, the
  `state=` suffixes unchanged), the frame diff shown, M1 byte-identical
  (`09b726b7…`), and the first differing frame **at or after** the first
  face-driven event. Any diff starting earlier or unexplainable by a
  lane/arm(gate/sub-change) is a stop-and-report.
- **Baseline inversion (FU321-L11):** the `:3701-3707` asserts are inverted and
  the replacement P7/P8 window/lockout assertions are pinned in the **same**
  commit; deleting or weakening without a replacement is NO-GO.
- **Census/README:** `60/80 = 75%` must survive (no new federated action-row
  wiring); the README title is recomputed only if a re-count proves otherwise.
  The register/errata discipline follows `AGENTS.md` (append-only, quoted
  errata).

## 7. Honest-stop rule

Any measurement or probe that contradicts the frozen prediction
(FU319 §6/FU321 §5) becomes a numbered open leg with its native owner — never
asserted, never explained away. If the patched out-of-tree probe diverges
before the first face-driven event or moves frames no face-gate reader can
explain, Stage 2 halts and reports (spec §6 golden rule). The phase-32
witness's driven timeline may move under the live face: measure it (FU323),
rule whether it re-pins, and write the reason — do not silently upgrade it.

## 8. Decisions (this spec)

1. The acceptance vehicle is the **undriven natural tape** (the natural pass on
   `M2_KEYS` verbatim, no added holds — the run FU319 §6 measured); the
   phase-32 drive only governs how the register's Layer-2 wording names the
   live run if needed.
2. **The move_attr metric is separately pinned** (FU319-L3); it never rides the
   face producer's commit.
3. The kickoff placement uses the **narrow install** (set `row43`/`+0x44` from
   the selected row's flags), not a routed `fifa96_match_run_record_anim_install`
   call (FU319 §5 writer 3; the `anim_timer` side effect stays collateral).
4. The focus point is the **default camera triple** only; non-default focus
   pointers are FU319-L1, untouched.
5. The `direct_face`-from-`anim_id` derivation is **NO-GO** (FU319 §5 rejected
   shortcut; the `+0x28 = 0` reset and the row-0x22 clear would re-invent `2`).
6. The inversion commit is atomic: the `:3701-3707` inversion, P7/P8, the
   golden re-pin, and the node's provenance note in the same commit.
7. The phase-32 driven witnesses re-pin only with measured evidence (§7).

## 9. Open items the wave settles

1. **FU322** — the exact inverted baseline + P7/P8 assertion design (what the
   window/lockout asserts pin, where, and what the natural-window cells are).
2. **FU323** — the measured M2 diff of a patched out-of-tree build (the natural,
   claim and offset passes; the first differing frame; the `state=` check;
   whether the phase-32 witnesses survive or re-pin).
3. **FU324** — the `move_attr` metric census fresh in Ghidra (readers of
   `+0x63 >> 16`/`>> 19`), the port's staging, the stride blast radius, and the
   sub-change's RED/GREEN design with its own measured diff.
4. **FU325** — the Stage-2 acceptance and governance (the Layer-2 live design,
   the golden recipe, the hazard list, the unit-pin plan), arguing from the
   measured numbers.

## Plan erratum (wave close, 2026-10-22)

The wave measured, and this spec's §1/§6/§8.1 premise is **corrected by it**
(append-only; the sections above stand as written):

Quote (§1, and §6 Layer-1): "after this phase the undriven natural tape pans
(the band record arrives facing its focus point, the row-04 angle gate passes
natively, arm B arms)". **Corrected (FU323, measured):** the mechanism is met —
the band record arrives facing the focus point and the gate passes — but arm B
is a **silent bind**: the undriven band record holds no control slot, the seed
switch takes the **no-slot arm** (FU316-L1's open leg), `v0 = 0`, and **no
granted frame ever has `pos ≠ (0,0,0)`**; the camera cells never differ from
the unpatched build. The undriven tape's honest post-producer baseline is
"faces live from step 9, a code-5 bind at granted f=1517, camera still zero"
(FU322's erratum re-shapes the inverted asserts accordingly; A4/A5's pan-window
asserts relocate to the **driven** claim/offset passes, which FU323 measured
re-pinning — the offset pass `G3` improves to `lane_at_pass 129` with the
plateau caveat gone, the claim pass moves to the no-slot arm's `(−0x120,0,0)`
shape). The pan vehicle is therefore the **driven witnesses**, and the
natural-pan future belongs to **FU316-L1** (the no-slot seed/travel leg).

Also measured: the first **golden** diff is frame **852** (the sit-1 half-time
kickoff draw divergence, 5 671 hash-only lines, `state=` suffixes unchanged,
M1 byte-identical on the patched engine) — §6's golden governance anchor is the
measured 9/52/852 chain, not a pre-pan camera frame (FU325's erratum re-words
the tripwires). And FU319-L3's predicted metric magnitude is **falsified**
(FU324: 0 moved frames; the metric sub-change is a measured no-op with a
contract pin).
