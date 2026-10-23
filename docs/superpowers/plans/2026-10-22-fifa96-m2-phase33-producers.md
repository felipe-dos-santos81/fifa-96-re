# FIFA 96 M2 phase-33 producers — the live record face

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Land the native `+0x43` record latch and the camera-focus staging live (FU314-L5's fix, FU319 §5's staging, verified measured by FU322–FU325), re-pin the moved witnesses and the M2 golden under the faithful-move policy, land the move_attr metric as its own contract-pinned no-op commit, run the Layer-2 live acceptance, and close the phase register.

**Architecture:** Five tasks: (1) the producer implementation with RED→GREEN unit pins and the measured witness/golden re-pin; (2) the re-shaped natural baseline + the relocated driven pan-window/lockout asserts; (3) the move_attr contract pin (a measured no-op); (4) the Layer-2 live acceptance (the driven variant — the natural clause is NOT-GO-as-measured); (5) the append-only close-out.

**Tech Stack:** C17/CTest, the null-backend harness, SDL3 `make game`, the env-gated camera trace, ffmpeg/ImageMagick for the captures.

**Spec:** `docs/superpowers/specs/2026-10-22-fifa96-m2-phase33-live-record-face-design.md` (read its **Plan erratum** first — the wave re-shaped the acceptance)

## Decisions carried into this plan (the frozen wave, FU322–FU325, errata included)

- **The staging is FU319 §5 verbatim** (the patch fidelity definition): a `uint8_t row43` beside `row44` in both structs; writer 1 `fifa96_match_run_record_anim_install` (`row.flags & 0x02`), writer 2 the successor (`0x8DF84` twin), writer 3 the **kickoff narrow install** (`fifa96_match_entities.c:565-568` — no routed full install), the clear `fifa96_match_action_22` stage 0 (`handlers.c:5214-5221`), and the mover staging `s.direct_face = r->row43`, `s.face_x/face_z = (int16_t)camera.pos_x/pos_z`. **No `move_attr` swap rides this** (FU324: the metric is its own task 3).
- **FU323 measured the real M2 move:** the unpatched probe reproduces the committed golden byte-for-byte; the patched build's first differing `frame=` line is **852** (the sit-1 half-time kickoff draw divergence), **5 671 hash-only lines** (contiguous 852..6522), `state=` suffixes and field order byte-unchanged, frames 1..851 identical, **M1 byte-identical on the patched engine**. Faces settle live from step 9; the first face-driven movement frame is 52; the camera cells **never differ undriven**.
- **The undriven tape does NOT pan — native-faithful.** The band record holds no control slot ⇒ the seed switch takes the **no-slot arm** ⇒ the code-5 event at granted f=1517 is a **silent bind** (FU321-L2's class); the camera stays `(0,0,0)` ×3 598. The natural-pan future is **FU316-L1** (the no-slot seed/travel leg), not this phase.
- **The driven witnesses get MORE robust and re-pin (FU323 §5):** the offset pass keeps `G3` better (`lane_at_pass 129 ≥ 1`, `(cam_dx,cam_dz) ≠ (0,0)`, the plateau caveat gone) with timing cells moved; the claim pass re-pins to FU323's measured claim tuple — **segment-aware capture required** (FU323's review erratum: the loose tables mix the claim's first segment with the offset pass's cells; probe.c's per-segment re-assignment).
- **FU324 falsified FU319-L3's magnitude** (0 straddles, 0 frames moved; the mutant also 0 — the stride clamp absorbs it) but confirmed the direction (the native metric is `0x8DC68`, the port stages `0x8DCD4`; no writer of the low word). Task 3 is a **contract pin** (RED-proofed by the recorded mutant run), golden-untouched, its own commit.
- **FU325's erratum governs Layer 2:** the "no driven variant" decision is void — the natural clause is NOT-GO-as-measured; the **driven variant (re-pinned per FU323 §5)** is the Layer-2 witness, trace-primary, AE-relative, sprite-only-bbox-caveated.

## Global Constraints

- `make check` green (strict `-Wall -Wextra -Werror`, ASan/UBSan; ~210 s → ≥600 s timeout) before every commit; work on `main`; commit and push per task.
- M1 `09b726b7…` immutable — `cmp` after every engine-touching task.
- M2 `8434cfab…` re-pins **only** in the commit that carries the golden move (task 1), with the written reason in `tests/test_engine_m2.c`'s provenance in the house form ("first differing line frame 852, 5 671 hash lines, `state=` suffixes unchanged"), the frame diff shown, and the register pin in task 5. Any diff starting **before** the measured face-driven chain (step 9/52/852) or any `state=` change is a **STOP-and-report**, not a re-pin (spec §6/§7; FU325's tripwires).
- The implementation matches FU319 §5 exactly — no extra staging, no `anim_id`-derived `direct_face` (NO-GO), no routed kickoff install, no non-default focus pointers (FU319-L1).
- The census holds `60/80 = 75%`; the README title is recomputed only if a fresh census says otherwise (task 5 checks).
- Docs are append-only with quoted errata; the SDD ledger lives at `.superpowers/sdd/phase33/progress.md` (git-ignored scratch) and is updated after every task.
- The ISO is read-only; captures go under `docs/screens/p33-*` following the p31/p32 note formats.

## Review Focus

1. **The patch fidelity in-tree** — the landed staging must be byte-equivalent to FU323's verified patch (nothing more); a silent metric fold or an extra writer is a review reject.
2. **The golden accounting** — the re-pin reason names the measured chain (852-chain), M1 cmp output quoted, and the pre-852 window proven identical.
3. **Segment-aware captures** — the claim pass's re-pin must not quote the offset pass's cells (FU323's erratum).
4. **The inversion asserts are measured-shape** — locked pins never re-pin on unrelated changes (FU322's A2 identity rule); the relocated pan-window asserts live on the driven passes, not the undriven window.
5. **The honest Layer-2 wording** — the natural clause is NOT-GO-as-measured; the driven witness is the live vehicle; no "the trace shows the offset" (the offset is a Layer-1 tuple claim).

---

### Task 1: The producer implementation, unit pins, and the measured re-pin

**Files:**
- Modify: `include/fifa96_engine/fifa96_match_entities.h`, `include/fifa96_engine/fifa96_match_run.h`, `src/fifa96_engine/fifa96_match_run.c`, `src/fifa96_engine/fifa96_match_entities.c`, `src/fifa96_engine/fifa96_match_handlers.c`
- Modify: `tests/test_engine_match_frame.c` (P-W1/W2/W3/CLR/MOV), `tests/test_action_locomotion.c` (P-TURN), `tests/test_engine_m2.c` (the claim/offset witness pins + the golden provenance + the `:3722-3724` site note)
- Test: the focused binaries, then the full suite

**Interfaces:**
- Consumes: FU319 §5 (the staging), FU323 (the measured tuples + the patch artifact `/tmp/opencode/fu323/fu323.patch` as the fidelity reference), FU325 §4 (the pin plan).
- Produces: the live face; the pinned writers/clear/staging/turn-arm; the re-pinned claim/offset witnesses; the re-pinned M2 golden. Task 2's baseline cites these.

- [ ] **Step 1: RED the unit pins** per FU325 §4: P-W1 (extend `test_record_anim_install_row44`, match_frame.c:3041), P-W2 (extend `test_record_row_successor_chain`, :3081), P-W3 (extend `test_kickoff_place_face_and_selector`, match_entities.c:1055), P-CLR (extend `test_action_22_runs_body` stage-0, match_handlers.c:4068; the shared-reset rider), P-MOV (a NEW settled-record fixture: `row43=2`, camera `(0,0,0)` → first `0x40` face step; the riders: `row43=0` no-turn, anti-`anim_id`-derivation, clear-freeze, far-target, int16 width), P-TURN (extend `test_step_direct_face`: the z-only focus arm + the native-value-2 equivalence). Run the focused tests; expected FAIL for the staging REDs (P-TURN may be GREEN-today — its RED is P-MOV).
- [ ] **Step 2: Implement FU319 §5 verbatim** (the cell, the three writers, the clear, the mover staging; the int16 truncation is the native width). Compare against `/tmp/opencode/fu323/fu323.patch` — equivalent or explain.
- [ ] **Step 3: GREEN the pins**; focused tests pass.
- [ ] **Step 4: Measure and re-pin the moved cells** (FU323 §tables, segment-aware): the claim pass's measured claim tuple and the offset pass's `lane_at_pass 129` + timing cells — assert the MEASURED values with FU323's doc as the source; update the `:3722-3724` site note to the re-shaped meaning (A1's bind claim) WITHOUT yet adding task 2's relocated asserts.
- [ ] **Step 5: The golden re-pin.** `./build/test_engine_m2 > /tmp/opencode/p33-m2.new`; `diff -u tests/golden/engine/m2-frames.txt /tmp/opencode/p33-m2.new | head -40`; count moved `frame=` lines; the `state=` check (`diff <(cut -d' ' -f3- …)`); first differing frame must be 852 and the window must match FU323's chain; M1 `cmp` byte-identical; then re-pin the golden and write the provenance reason in the house form.
- [ ] **Step 6: GREEN the suite.** `make check` 111/111; M1 `cmp`; M2 `cmp` (now vs the new pin); tree sweep.
- [ ] **Step 7: Commit and push** `fix(fu314): the live record face — the +0x43 latch and the focus staging`.
- [ ] **Step 8: Ledger** — `.superpowers/sdd/phase33/progress.md`: the task, the pins, the measured cells, the golden sha (old→new).

### Task 2: The re-shaped natural baseline and the driven pan window

**Files:**
- Modify: `tests/test_engine_m2.c` (the `:3701-3707`/`:3722-3724` assert set per FU322's errata; the sampled cells; the labels `:6035-6057`), possibly `tests/test_engine_match_frame.c` (FU322's H8-H11 sequenced moves — verify vs FU323 first; break → same commit)

**Interfaces:**
- Consumes: FU322 (the design + the errata: A1-A5 re-shaped, A4/A5 relocated to the driven passes), FU323's measured cells (the first-bind/left-zero window, the settled-face census, the `zero_frames`/`left_zero` cells), Task 1's landed producer.
- Produces: the honest post-producer baseline; the P7/P8-analog pan window/lockout asserts on the driven targets.

- [ ] **Step 1: Read FU322's errata** and map every assert change onto the current line map (the reviewer's drift warning applies — fresh line numbers this session).
- [ ] **Step 2: Land A1-A3** (the bind claim; the zero-frames identity; the window pin with the `M2_NAT_FACE_FIRST_LO/HI` constants from FU323's measured chain + the declared slack). The exact measured step is recorded, never pinned.
- [ ] **Step 3: Relocate A4/A5** (the pan window + lockout) onto the driven claim/offset passes per FU322's erratum: `segments`/`first_motion_f`/run length/stop place/decay/`pan_max` from the event frame's own recorded seed (arm-agnostic, zero-seed-proof); the lockout identities (`y_nonzero_frames == 0`, `goal_armed_any == 0`, `oob_frames == 0`, the trace `ev == 0`/`follow == 0` lines).
- [ ] **Step 4: The labels and the honest mode.** The P6–P8 label text (`:6049-6054`) and the P6-P8 comment (`:2293`) state the measured post-producer reality; the no-ISO degraded mode stays recorded-only until measured (FU322-L10) — verify `test_engine_m2_noiso` still skips/passes honestly.
- [ ] **Step 5: The sequenced hazards** (FU322's H1-H12): verify each vs the current tree and FU323's measurements; move in this commit only what must (break → same commit), record the survivors as verified.
- [ ] **Step 6: GREEN the suite + the goldens** (`make check`; M1/M2 `cmp` vs task 1's pins).
- [ ] **Step 7: Commit and push** `test(fu322): the re-shaped natural baseline and the driven pan window`.
- [ ] **Step 8: Ledger.**

### Task 3: The move_attr metric contract pin (measured no-op)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` (the mover staging `:521-522` — the `0x8DC68` block-A distance for `>>16`; the `>>19` stride attr = `distance >> 3`), `tests/test_engine_match_frame.c` or `tests/test_action_locomotion.c` (the contract pin)

**Interfaces:**
- Consumes: FU324 (the census, the 0-diff measurement, the mutant RED proof, the clamp analysis), FU319 §4's staging row.
- Produces: the metric staged from the native metric; a contract pin that is RED-proofed by the recorded mutation run; no golden move.

- [ ] **Step 1: RED the contract pin** — pin the staged `>>16` value equals `fifa96_entity_distance(...)` and the `>>19` equals `distance >> 3` at a measured tuple; before the swap the staged `0x8DCD4` value differs on a chosen input (or reuse FU324's recorded mutant output as the RED evidence if the pin cannot be RED naturally).
- [ ] **Step 2: Swap the staging** to the block-A metric (both `>>16` and `>>19` per FU324's semantics).
- [ ] **Step 3: GREEN + the no-op proof:** `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` (the pin from task 1) — byte-identical; M1 `cmp`; `make check` 111/111.
- [ ] **Step 4: Commit and push** `fix(fu319): the move_attr metric staged from the block-A distance`.
- [ ] **Step 5: Ledger** (note FU324-L5: the attr sensitivity returns with `+0x7B` nibble staging — re-measure then).

### Task 4: The Layer-2 live acceptance (the driven variant)

**Files:**
- Create: `docs/screens/p33-l2-notes.md` (+ the `p33-l2-*` capture/trace artifacts per the p31/p32 formats)

**Interfaces:**
- Consumes: FU325 §6 + its errata (the fallback rule triggered: the driven witness is the Layer-2 vehicle), FU317/FU321 §4 (the procedure), the task 1-2 re-pinned drive timeline.
- Produces: the live acceptance note + the artifacts task 5 registers.

- [ ] **Step 1: Run the live driven witness** (`DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p33-cam-trace.log make game`; the phase-8 walkthrough; the tape-equivalent prefix; the tick-synced act-1 KICK; the drive started at phase 2 + 20 ticks, phase-2-relative indices, never absolute).
- [ ] **Step 2: Capture the pairs** (same-clock pan/still pairs inside the displayed second and the trace's delta run; crop `960x591+0+0`; `compare -metric AE`; the HUD crop AE 0; the bbox caveat).
- [ ] **Step 3: Extract the trace** and check the gates against the re-pinned measured cells (the pan window on the driven pass: one delta run per segment, `trk`, `pan 0→4`, `pos_y=0`, `follow=0`; the seed read from the event frame's own cells — arm-agnostic per FU322).
- [ ] **Step 4: Write the note** with the honest verdict: **the driven pan reproduced live under the live face**; the natural (undriven) clause recorded as NOT-GO-as-measured with the silent-bind numbers; the segment-aware anchors; the exact commands; the timing limits if any (the FU321-L4 class).
- [ ] **Step 5: If the live timing cannot be reproduced**, land the honest timing waiver with the exact limits (what was tried, the trace/absence) — never a invented verdict.
- [ ] **Step 6: Commit and push** `docs(fu325): the live record face acceptance (the driven variant)`.
- [ ] **Step 7: Invariants:** no code/test/golden change in this task; M1/M2 `cmp` unchanged.

### Task 5: The close-out register

**Files:**
- Modify: `docs/ENGINE.md` (append the phase-33 register), `docs/PROGRESS.md` (append the narrative), the phase-33 spec (append the close-out erratum if any)
- Test: the final verification commands

**Interfaces:**
- Consumes: tasks 1-4's records; the frozen wave FU322-FU325 (+ FU318/FU319/FU321's phase-33 errata).
- Produces: the frozen record; the next-phase hand-off.

- [ ] **Step 1: Append the register** (append-only, quoted errata style): the verdict (the live face landed; the measured 852-chain re-pin; the undriven honest baseline; the driven witnesses more robust); the unit pins; the metric contract pin; the Layer-2 evidence; the wave verdicts + errata; the legs (FU322-L1..L12, FU323-L1..L7, FU324-L1..L7, FU325-L1..L7) with owners; **FU316-L1 promoted** as the natural-pan owner leg (the no-slot seed/travel).
- [ ] **Step 2: The PROGRESS narrative** — the face is live, the pan story's honest shape, the golden re-pin's reason, what phase 34's candidates are (FU316-L1 the no-slot seed/travel; FU316-L9 the unarmed mirror; FU316-L8 the row-05 carrier camera event; FU315-L1/L2 the claim tail).
- [ ] **Step 3: The invariants:** fresh census count (`60/80` expected — recompute and fix the README title in this commit if it moved); `make check` 111/111; M1/M2 `cmp`.
- [ ] **Step 4: Commit and push** `docs(fu314): the phase-33 close-out — the live record face`.
- [ ] **Step 5: Final ledger + report.**
