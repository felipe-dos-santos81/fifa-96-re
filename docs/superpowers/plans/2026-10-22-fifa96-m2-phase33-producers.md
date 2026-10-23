# FIFA 96 M2 phase-33 producers — the live record face

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Land the native `+0x43` record latch and the camera-focus staging live (FU314-L5's fix, FU319 §5's staging, measured by FU322–FU325) in **one atomic producer commit** (the implementation + unit pins + the re-shaped baseline inversion + the segment-aware witness re-pins + the M2 golden re-pin with the stratum-set gate), then land the move_attr metric as its own contract-pinned no-op commit, run the Layer-2 live acceptance (the driven variant), and close the phase register.

**Architecture:** Four tasks: (1) the atomic producer commit (implementation + pins + inversion + re-pins + golden); (2) the move_attr contract pin (a measured no-op); (3) the Layer-2 live acceptance (the driven variant — the natural clause is NOT-GO-as-measured); (4) the append-only close-out.

**Tech Stack:** C17/CTest, the null-backend harness, SDL3 `make game`, the env-gated camera trace, ffmpeg/ImageMagick for the captures.

**Spec:** `docs/superpowers/specs/2026-10-22-fifa96-m2-phase33-live-record-face-design.md` — read its **Plan erratum (wave close)** first; it amends §1/§5/§6/§8 (the P1/P2 sketch is re-shaped into task 1's atomic commit).

**Name decode (used throughout):** **A1–A5** = FU322's replacement-assert set (its §4.1); **H1–H12** = FU322's hazard list (its §6); **FU323 §5's tables** = the measured witness tuples in `docs/ghidra/FU323_r2l7_m2_probe.md` §5 — but the **segment-aware authority is FU322's erratum 2**, because FU323's loose tables mix each pass's first dispatch with its LAST segment's cells (FU323's own erratum); **the golden recipe** = FU323 §6 (normative).

## Decisions carried into this plan (the frozen wave, FU322–FU325, errata included)

- **The staging is FU319 §5 verbatim** (the fidelity definition): a `uint8_t row43` beside `row44` in both structs; writer 1 `fifa96_match_run_record_anim_install` (`row.flags & 0x02`), writer 2 the successor (`0x8DF84` twin), writer 3 the **kickoff narrow install** (`fifa96_match_entities.c:565-568` — no routed full install), the clear `fifa96_match_action_22` stage 0 (`handlers.c:5214-5221`), and the mover staging `s.direct_face = r->row43`, `s.face_x/face_z = (int16_t)camera.pos_x/pos_z`. **No `move_attr` swap rides this** (it is task 2's).
- **FU323 measured the real M2 move** (its unpatched probe reproduces the committed golden byte-for-byte): the patched build's first differing `frame=` line is **852** (the sit-1 half-time kickoff draw divergence), **5 671 hash-only lines** (contiguous 852..6522), `state=` suffixes and field order byte-unchanged, frames 1..851 identical, **M1 byte-identical on the patched engine**. Faces settle live from step 9; the first face-driven position movement is frame 52.
- **The undriven tape does NOT pan — native-faithful.** The band record holds no control slot ⇒ the no-slot seed arm ⇒ `v0 = 0` ⇒ the code-5 event at granted f=1517 is a **silent bind** (FU323-L1): camera `pos` never moves, but the camera **event state engages** from f=1517 — `camera_zero_frames` 1517 / `camera_left_zero` 2081, i.e. the standing `:3722` assert's **measured RED**, the inversion's own evidence. The natural-pan future is **FU316-L1** (the no-slot seed/travel leg), not this phase.
- **The driven witnesses re-pin with REAL pans — two chained segments each** (the segment-aware authority: FU322's erratum 2). Claim pass: the row-04 `−0x90` one-shot (bind f=1483, `trk=2` window f=1483..1552, stop f=1511) plus a **new code-3 walk camera event** the frozen world did not have (gate f=1552, event f=1553, `trk=1`, stop f=1581); totals 54 changed / 2 116 off-zero frames, stop `(−0x120,0,0)`. Offset pass: gate f=1552 (`lane_x` 129 = bound 129, `cam_dz` −4, Δ=0), 2 segments, stop `(−0x120,0,0)`. **Pin BOTH segments per pass** (FU323's erratum 1) and capture traces segment-aware.
- **FU324:** the two metric cores are the same computation (4 000 800 pairs, 0 diffs; the mutant also moves 0 frames — the stride clamp absorbs it), so the metric sub-change is a **contract pin GREEN at commit** whose guard is the recorded mutant run, never an invented failing input; FU319-L3's magnitude is falsified (erratum leg FU324-L1); the `>>19` pacing-slack rationale re-bases on measured variance (FU322's erratum 3).
- **FU322-L11 as re-shaped: one atomic producer commit.** With the engine live the standing `tests/test_engine_m2.c:3722-3724` asserts fire (the measured RED) and the golden moves in the same build — the inversion (A1–A3 bind-anchored; A4/A5 relocated to the driven passes), P7/P8, the golden re-pin and the provenance note cannot be split across commits without a RED intermediate.
- **FU325's erratum governs Layer 2:** the "no driven variant" decision is void — the natural clause is NOT-GO-as-measured; the **driven variant (re-pinned per the segment-aware tables)** is the Layer-2 witness, trace-primary, AE-relative, sprite-only-bbox-caveated.

## Global Constraints

- `make check` green (strict `-Wall -Wextra -Werror`, ASan/UBSan; ~210 s → ≥600 s timeout) before every commit; **no commit may leave the suite RED** — task 1 is atomic for exactly this reason; work on `main`; commit and push per task.
- M1 `09b726b7…` immutable — `cmp` after every engine-touching task.
- M2 `8434cfab…` re-pins **only** in task 1's commit, with the written reason in `tests/test_engine_m2.c`'s provenance in the house form ("first differing line frame 852, 5 671 hash lines, `state=` suffixes unchanged"), the frame diff shown, and the register pin in task 4. Normative recipe: FU323 §6. **Tripwires:** any first-diff frame ≠ 852, any `state=` change, or any moved frame FU323's build does not reproduce → **STOP-and-report**, never a re-pin (spec §6/§7; FU325's re-worded tripwires).
- The stratum-set gate: the additivity gate is **replaced** by the stratum-set gate in the same commit (Outcome B — the first diff 852 sits inside the v21 2 522-line prefix); FU323 measured the stratum set unchanged (`state=1` ×951, `state=19` ×2 620, `state=2` ×2 100).
- The implementation matches FU319 §5 exactly — no extra staging, no `anim_id`-derived `direct_face` (NO-GO), no routed kickoff install, no non-default focus pointers (FU319-L1), and **no metric fold** (task 2's change).
- The census holds `60/80 = 75%`; the README title is recomputed only if a fresh census says otherwise (task 4 checks).
- Docs are append-only with quoted errata; the SDD ledger lives at `.superpowers/sdd/phase33/progress.md` (git-ignored scratch) and is updated after every task.
- The ISO is read-only; captures go under `docs/screens/p33-*` following the p31/p32 note formats.

## Review Focus

1. **Patch fidelity in-tree** — the landed staging must be equivalent to FU323's verified patch (`/tmp/opencode/fu323/fu323.patch`; FU319 §5 is the source of truth, the patch the cross-check); a silent metric fold or an extra writer is a review reject.
2. **The atomic commit's completeness** — the inversion set (A1–A3 + the relocated A4/A5), the golden, the provenance note and the witness re-pins all in task 1's commit; nothing RED at any commit point.
3. **Segment-awareness** — both segments pinned per driven pass; no cell quoted from FU323's loose tables (its erratum).
4. **Locked pins never re-pin on unrelated changes** — FU322's A2 identity rule; the relocated A4/A5 assert per-segment `T(s)` arithmetic from the pass's own recorded seeds (arm-agnostic, zero-seed-proof), never one-segment/one-literal forms.
5. **The honest Layer-2 wording** — the natural clause is NOT-GO-as-measured; the driven witness is the live vehicle; no "the trace shows the offset" (the offset is a Layer-1 tuple claim).

---

### Task 1: The atomic producer commit (implementation + pins + inversion + re-pins + golden)

**Files:**
- Modify: `include/fifa96_engine/fifa96_match_entities.h`, `include/fifa96_engine/fifa96_match_run.h`, `src/fifa96_engine/fifa96_match_run.c`, `src/fifa96_engine/fifa96_match_entities.c`, `src/fifa96_engine/fifa96_match_handlers.c`
- Modify: `tests/test_engine_match_frame.c` (P-W1/W2/MOV), `tests/test_engine_match_entities.c` (P-W3), `tests/test_engine_match_handlers.c` (P-CLR), `tests/test_action_locomotion.c` (P-TURN), `tests/test_engine_m2.c` (the inversion + the labels + the witness re-pins + the golden provenance)
- Test: the focused binaries, then the full suite

**Interfaces:**
- Consumes: FU319 §5 (the staging); FU322's errata 1–5 (the assert set + the stratum gate); FU322's erratum 2 (the segment-aware witness tables — the numbers source); FU323 §6 (the golden recipe); FU325 §4 (the pin plan, its two-step RED discipline).
- Produces: the live face; the pinned writers/clear/staging/turn-arm; the re-shaped baseline; the re-pinned witnesses; the re-pinned M2 golden. Everything task 2–4 cite.

- [ ] **Step 1: Declare-first (FU325 §4's two-step RED).** Add the struct fields (`row43` in both structs) and the new sampler cells the inversion needs (`first_left_zero_f`, the delta-run set replayed on the undriven run, the Y/Z counters) **compile-only** — compile green before any assert lands.
- [ ] **Step 2: RED the unit pins.** P-W1 extend `test_record_anim_install_row44` (`tests/test_engine_match_frame.c:3041`; RED = a row `0x00`/`0x26`/`0x80→0` install leaves the declared-zero `row43` at 0, expected 2). P-W2 extend `test_record_row_successor_chain` (`tests/test_engine_match_frame.c:3081`; RED = `0x0F→0x00` leaves 0, expected 2). P-W3 extend `test_kickoff_place_face_and_selector` (`tests/test_engine_match_entities.c:1055`; RED = every placed record 0, expected 2; the narrow write, no routed install). P-CLR extend `test_action_22_runs_body`'s stage-0 block (`tests/test_engine_match_handlers.c:4057`, the stage-0 block ~`:4068`; RED = a staged 2 stays 2; rider: the shared reset must NOT clear). P-MOV a NEW settled-record fixture in `tests/test_engine_match_frame.c` (`row43=2`, camera `(0,0,0)` → RED = the face frozen at 0x100, GREEN = the first `0x40` step; riders: `row43=0` no-turn, anti-`anim_id`-derivation, clear-freeze, far-target, int16 width). P-TURN extend `test_step_direct_face` (`tests/test_action_locomotion.c:118-149`: the z-only focus arm + the native-value-2 equivalence; may be GREEN-today — its RED is P-MOV). Run the focused tests; the staging REDs fail as designed.
- [ ] **Step 3: Implement FU319 §5 verbatim** (the cell, the three writers, the clear, the mover staging; the int16 truncation is the native width). Cross-check against `/tmp/opencode/fu323/fu323.patch` — equivalent or explain.
- [ ] **Step 4: GREEN the unit pins**; focused tests pass.
- [ ] **Step 5: The re-shaped inversion (FU322's errata 1–2).** A1 `camera_left_zero >= 1` with the comment re-worded to the measured consequence ("the row-04 gate passes once and binds a record with a zero seed — the FU323-L1 silent-bind baseline"). A2 the identity `camera_zero_frames == first_left_zero_f` (measured 1517 == 1517). A3 `first_left_zero_step > phase2_step` + the LO/HI window anchored to FU323's measured **bind** (granted f=1517 / engine step 5065), the slack re-based on measured pacing variance (FU322's erratum 3 — no metric transient exists to cover); the exact step recorded, never pinned. **A4/A5 relocated to the driven claim/offset passes:** two chained segments each, the per-segment `T(s)` arithmetic asserted from the pass's own recorded seeds (arm-agnostic, zero-seed-proof), `row04_after_event_frames == 0`, the lockout identities (`stop_y == 0`, `y_nonzero_frames == 0`, `goal_armed_any == 0`, `oob_frames == 0` — measured 0 on the driven passes); the undriven bounds stay recorded-only (not in FU323's undriven dump — FU322's erratum 2). Labels: the `:2293` comment and the `:6035-6057` label text state the measured reality; the no-ISO degraded mode stays recorded-only (FU322-L10) — verify `test_engine_m2_noiso` stays honest.
- [ ] **Step 6: The segment-aware witness re-pins.** Both driven passes' moved tuples asserted from **FU322's erratum 2 tables** (both segments per pass): claim bind f=1483/window/stop 1511 + the code-3 event gate f=1552/event f=1553/stop 1581, totals 54/2 116, stop `(−0x120,0,0)`; offset gate f=1552, `lane_x` 129, Δ=0, 2 segments, stop `(−0x120,0,0)`.
- [ ] **Step 7: The golden re-pin (FU323 §6's recipe).** Build; run; `diff -u tests/golden/engine/m2-frames.txt <new>`; the first differing frame must be **852** and the moved set must match FU323's measured chain (5 671 hash-only lines, contiguous 852..6522, suffixes unchanged); land the stratum-set gate (the strata 951/2620/2100, replacing the additivity gate); M1 `cmp` byte-identical; re-pin the golden; write the provenance reason in the house form. Any tripwire hit (Global Constraints) → STOP.
- [ ] **Step 8: GREEN the suite.** `make check` 111/111; M1/M2 `cmp` (vs the new pins); tree sweep.
- [ ] **Step 9: Commit and push** `fix(fu314): the live record face — the +0x43 latch, the focus staging, and the measured re-pin` (the controller's call: `fix` — the change repairs the port's staged-zero suppression of a live native behaviour).
- [ ] **Step 10: Ledger** — `.superpowers/sdd/phase33/progress.md`: the task, the pins, the measured cells, the golden sha (old→new).

### Task 2: The move_attr metric contract pin (measured no-op)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` (the mover staging `:521-522` — stage the high word from the block-A `0x8DC68` distance; the `>>19` stride attr = `distance >> 3`), `tests/test_engine_match_frame.c` or `tests/test_action_locomotion.c` (the contract pin)

**Interfaces:**
- Consumes: FU324 (the census, the 0-diff measurement, the mutant sensitivity run, the clamp analysis), FU319 §4's staging row.
- Produces: the metric staged from the native metric source; a contract pin GREEN at commit; no golden move.

- [ ] **Step 1: Land the contract pin** — assert the staged `>>16` value equals `fifa96_entity_distance(...)` and the `>>19` equals `distance >> 3` at a measured tuple. Per FU324 §6 this pin is **GREEN at commit** (the port already satisfies it numerically — the two cores compute identically); its guard is FU324's **recorded mutant run** (the sensitivity evidence in the doc), never an invented failing input; if a natural RED input exists, name it — otherwise the pin's RED story is the mutant, stated at the assert site.
- [ ] **Step 2: Align the staging source** to the native block-A distance (`0x8DC68`) for both `>>16` and `>>19` — the value does not change; the staging source does.
- [ ] **Step 3: The no-op proof.** `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` (vs task 1's pin) — byte-identical; M1 `cmp`; `make check` 111/111.
- [ ] **Step 4: Commit and push** `fix(fu319): the move_attr metric staged from the block-A distance (the contract pin)`.
- [ ] **Step 5: Ledger** (note FU324-L5: the attr sensitivity returns with `+0x7B` nibble staging — re-measure then).

### Task 3: The Layer-2 live acceptance (the driven variant)

**Files:**
- Create: `docs/screens/p33-l2-notes.md` (+ the `p33-l2-*` capture/trace artifacts per the p31/p32 formats)

**Interfaces:**
- Consumes: FU325 §6 + its errata (the fallback rule triggered: the driven witness is the Layer-2 vehicle), FU317/FU321 §4 (the procedure), task 1's re-pinned segment-aware timeline.
- Produces: the live acceptance note + the artifacts task 4 registers.

- [ ] **Step 1: Run the live driven witness** (`DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p33-cam-trace.log make game`; the phase-8 walkthrough; the tape-equivalent prefix; the tick-synced act-1 KICK; the drive started at phase 2 + 20 ticks, phase-2-relative indices, never absolute).
- [ ] **Step 2: Capture the pairs** (same-clock pan/still pairs inside the displayed second and the trace's delta run; crop `960x591+0+0`; `compare -metric AE`; the HUD crop AE 0; the bbox caveat). **Segment-aware:** anchor each capture to a named segment's window (task 1's two-segment shape), never a mixed span.
- [ ] **Step 3: Extract the trace** and check the gates against the re-pinned measured cells (one delta run per segment, `trk` per segment's event, `pan 0→4`, `pos_y=0`, `follow=0`; the seed read from the event frame's own cells — arm-agnostic per FU322).
- [ ] **Step 4: Write the note** with the honest verdict: **the driven pan reproduced live under the live face**; the natural (undriven) clause recorded as NOT-GO-as-measured with the silent-bind numbers; the segment-aware anchors; the exact commands; the timing limits if any (the FU321-L4 class).
- [ ] **Step 5: If the live timing cannot be reproduced**, land the honest timing waiver with the exact limits (what was tried, the trace/absence) — never an invented verdict.
- [ ] **Step 6: Commit and push** `docs(fu325): the live record face acceptance (the driven variant)`.
- [ ] **Step 7: Invariants:** no code/test/golden change in this task; M1/M2 `cmp` unchanged.

### Task 4: The close-out register

**Files:**
- Modify: `docs/ENGINE.md` (append the phase-33 register), `docs/PROGRESS.md` (append the narrative), the phase-33 spec (append the close-out erratum if any)
- Test: the final verification commands

**Interfaces:**
- Consumes: tasks 1-3's records; the frozen wave FU322-FU325 (+ FU319's phase-33 erratum).
- Produces: the frozen record; the next-phase hand-off.

- [ ] **Step 1: Append the register** (append-only, quoted errata style): the verdict (the live face landed in the atomic commit; the measured 852-chain re-pin with the stratum-set gate; the undriven honest baseline: faces live + the silent bind, camera pos zero; the driven witnesses re-pinned, two chained segments each); the unit pins; the metric contract pin (GREEN at commit, the mutant guard); the Layer-2 evidence; the wave verdicts + errata (FU322's re-shape, FU323's silent bind + loose-table caveat, FU324's falsification, FU325's fallback trigger); the legs (FU322-L1..L12, FU323-L1..L7, FU324-L1..L7, FU325-L1..L7) with owners; **FU316-L1 promoted** as the natural-pan owner leg (the no-slot seed/travel).
- [ ] **Step 2: The PROGRESS narrative** — the face is live, the pan story's honest shape, the golden re-pin's reason, the next-phase candidates (FU316-L1 the no-slot seed/travel; FU316-L9 the unarmed mirror; FU316-L8 the row-05 carrier camera event; FU315-L1/L2 the claim tail).
- [ ] **Step 3: The invariants:** fresh census count (`60/80` expected — recompute and fix the README title in this commit if it moved); `make check` 111/111; M1/M2 `cmp`.
- [ ] **Step 4: Commit and push** `docs(fu314): the phase-33 close-out — the live record face`.
- [ ] **Step 5: Final ledger + report.**
