# FU-325 — the Stage-2 acceptance and golden governance for the live record face (phase-33 R2-L7)

**Phase/Provenance:** phase-33 recon-ahead wave, Stage-1 **Task 4 of 4** (`docs/superpowers/plans/2026-10-22-fifa96-m2-phase33-recon-ahead.md:85-98`), serving `docs/superpowers/specs/2026-10-22-fifa96-m2-phase33-live-record-face-design.md` §5 (Structure), §6 (Acceptance), §7 (honest-stop), §8 (Decisions). **Repo-static design, no Ghidra read, no probe, no measurement:** every native fact is **carried** with its owner doc (FU319 §2/§3/§5/§6, FU321 §5/§6, FU314 §2.3/§5/L5 + review erratum, FU-185-L3) — the FU321 §0 carried-facts mode; every port/test claim is a first-hand `file:line` read this session (§11). Sibling numbers are **forward-cited by name** — FU322 (the inversion design), FU323 (the measured M2 diff), FU324 (the metric sub-change) — and this doc writes every quantity they own as "the measured value", never a number (the plan's Review Focus 5, no circular evidence). The only repo write is this draft; no git; the ISO untouched; scratch under `/tmp/opencode/fu325/`.

---

## 1. The question

> **Given the frozen producer (FU319 §5) and the spec §6 acceptance clauses, what exactly does the Stage-2 producer plan implement: the RED→GREEN unit-pin plan (three writers, the clear, the mover staging, the turn arm), the M2 golden re-pin recipe with its stop-and-report tripwires, the Layer-2 live design (driven variant or not under the live face), and the hazard list — while asserting no number the parallel wave slices own?**

## 2. The one-page answer

The Stage-2 plan implements **six pins across four existing CTest targets** (§4): three writer pins (install, successor, kickoff narrow write), the row-0x22 clear pin, the mover-staging pin (the new frame-path fixture — this is also the turn arm's RED, because the turn function itself is already pinned), and the turn-arm coverage extensions (GREEN-today by construction). Each pin's RED is the **staged-zero value of the pre-change tree** (declared-but-unwritten `row43` = 0; the mover's `direct_face = 0` freezing every face) — a two-step RED: declare the field, run RED, add the writer, run GREEN.

The golden recipe (§5) is FU321 §5.3's command chain run **verbatim** in the P2 commit, hardened by two additions: the first differing frame must be **exactly** the frame FU323 measured on the same patch (the acceptance reproduces an independent measurement; it never re-derives one), and the moved-line count/state check must come out `hash=`-only with every `state=` suffix unchanged. Seven tripwires (§5.3) convert "explainable diff" into commands with expected output shapes; any tripwire firing is a **stop-and-report** (spec §7), never a rationalized re-pin.

Layer-2 (§6) runs the **natural game run — no driven variant is required** under the live face. The phase-31/32 drives existed *because the face was frozen*: the undriven tape's only band entry arrived from the east and failed the angle gate (Δ 0x200 > 0xAB, FU314 §5); with the latch live the same entry takes the focus branch and passes (FU319 §6: face `0x300` at the band, Δ = 0 — **re-measured by FU323**). Spec §8.1 already fixes the undriven tape as the vehicle. The driven FU320 replay is the **fallback only**, keyed on FU323's measured verdicts (§6.2), replayed per the phase-32 procedure unchanged. The trace is primary (FU321 §4.5), the AE is the relative corroboration, the whole-scene bbox stays the documented sprite-only caveat.

The hazard list (§7) carries nine items, each with a tripwire: the successor's inherited one-frame placement (FU319-L4), the value-equivalence of the two unported team-placement install routes, the m-42/link fixture class, the claim ordering, the controlled fallback, the zero-seed silent event, the zero-delta knife-edge inversion under a settled face, the census/README invariants, and the honest-stop rule itself.

## 3. The bindings table

| # | Binding | What it fixes for this slice |
|---|---|---|
| B1 | FU319 §2 (the census: write sites `0x6E6C4`/`0x8DFBA`/`0x8540C`, read sites `0x7BF89`/`0x8E29C`) | the writer/clear pins' fidelity definition — three native write sites, one clear |
| B2 | FU319 §5 (the minimal staging) | the patch shape P1 must reproduce exactly (the probe's fidelity definition) |
| B3 | FU319 §6 (the prediction) | the Layer-1 numbers are FU323's to re-measure (spec §6); cited, never re-asserted |
| B4 | FU319 §7 L1/L2/L4/L5 | the legs this slice must not silently fix (§7) |
| B5 | FU321 §5 (the re-pin procedure + §5.2 house form + §5.4 fixtures) | the golden recipe's verbatim core |
| B6 | FU321 §6 (the hazards) + §8 L2/L11 | the hazard list's carried items |
| B7 | FU314 L5 (`:129`) + the review erratum's stage-2 consequence (`:174`) | the closed defect and the robust-route clause this phase lands |
| B8 | `docs/screens/p31-l2-notes.md` + `p32-l2-notes.md` | the Layer-2 capture formats, the timing lessons, the trace-primary rule |
| B9 | `tests/test_action_locomotion.c:118-149`, `tests/test_engine_match_frame.c:7651-7715`, `tests/test_engine_m2.c:3718-3724` | the pre-existing pin surfaces reused/extended |

## 4. The unit-pin plan (RED→GREEN per change)

**The two-step RED discipline.** For a *new cell* the first RED cannot be a failing assert (the field does not compile yet). The honest sequence is: (i) declare `row43` zero-initialized on the pool record and the staging record (a compile-only step, no writer); (ii) run the extended pins — each fails on the **staged-zero value of today's tree** (the RED named below); (iii) add the writer/staging — the pin goes GREEN. A RED expressed as a compile error or as "the field exists and nothing reads it" is not a RED; the executor records the failing assert's golden/actual pair per pin in the task report (the phase-31/32 task-report convention).

**Patch fidelity preamble.** The P1 patch is exactly the FU319 §5 shape: the two install writers (`match_run.c:1029`/`:1067` area), the kickoff narrow write (`match_entities.c:565-568`), the clear (`match_handlers.c:5209-5221`), the mover staging + the staging-load/drain write-back pair (`match_run.c:1413`/`:498`/`:511`/`:1561`). The `row43` staging load is the **pool pass-through** `r->row43 = e->row43` — *not* the `match_run_row_terminal_flag` derivation shape (`match_run.c:1006-1012`, `:1352`), which is the spec §8.5 NO-GO. Nothing else changes; P3's metric swap is out of this build (spec §8.2).

### 4.1 The six pins

| pin | change (the only edit) | test file / fixture | harness | RED today (the staged-zero value that fails) | status |
|---|---|---|---|---|---|
| **P-W1** install | `fifa96_match_run_record_anim_install`: `e->row43 = (uint8_t)(row.flags & 0x02u);` beside the row44 write (`match_run.c:1029`; native `0x6E6C4`, FU319 §2.2/§5 W1) | extend **`test_record_anim_install_row44`** (`tests/test_engine_match_frame.c:3041-3072`) — **pre-existing pin reuse**, the rows are already installed there (0x00/0x5E/0x26/0x80→0) | none | after declaring the field: install row 0x00 → `e->row43 == 0`, expected **2** (flags 0x07); same RED on 0x26 and on the id-clamped 0x80→0. Row 0x5E (flags 0x00) asserts 0 both ways — it is the negative half, not the RED | extended |
| **P-W2** successor | `fifa96_match_run_record_anim_successor`: `e->row43 = (uint8_t)(next_row.flags & 0x02u);` beside the row44 write (`match_run.c:1067`; native pool twin `0x8DFBA`, FU319 §5 W2) | extend **`test_record_row_successor_chain`** (`tests/test_engine_match_frame.c:3081-3114`) — **pre-existing pin reuse** | none | chain 0x0F→0x00 leaves `e->row43 == 0`, expected **2** (row 0x00 flags 0x07); the terminal no-op must keep it (assert `row43` unchanged); the mirror 0x14→0x0F lands 0 | extended |
| **P-W3** kickoff narrow write | `fifa96_match_entities_kickoff_place`: at the selector site (`match_entities.c:565-568`) set `e->row43` from the **selected** row's flags (`active ? 0 : 0x26` → but the selector may keep 0x62/special or fall back 0; read the selected row, not the request); native `0x79C13 CALL 0x6E598` (FU319 §5 W3); **do not** route `fifa96_match_run_record_anim_install` (spec §8.3: the `anim_timer`/`aux_seen` side effects are collateral) | extend **`test_kickoff_place_face_and_selector`** (`tests/test_engine_match_entities.c:1055-1074`) — **pre-existing pin reuse**: it stages the kept-special 0x62, the fallback 0x28→0, and an inactive record (→0x26) — all three selected rows are flags 0x07 → **2** | none | after kickoff_place: `e->row43 == 0` on every record, expected **2** on all of them (incl. `pool.team[TEAM1].records[0]`) | extended |
| **P-CLR** the clear | `fifa96_match_action_22` stage 0: `r->row43 = 0; e->row43 = 0;` at the `0x8540C` site (the FU185-L3 comment today at `match_handlers.c:5218`; the stage-0 block `:5209-5221`; native `0x8540C`, FU319 §2.2) | extend **`test_action_22_runs_body`** (`tests/test_engine_match_handlers.c:4057-4139`), the stage-0-active block (`:4068-4087`) | none | stage `row43 = 2` on the staging record **and** the pool entity (a declared fixture input, or a real install of row 0x26 on `f.mr.entities.team[0].records[1]`); run stage 0 → both still **2**, expected **0** | extended |
| **P-MOV** the mover staging (the turn arm's RED) | `match_run_record_mover`: the staging load `r->row43 = e->row43` (beside `match_run.c:1413`), `s.direct_face = r->row43`, `s.face_x = (int16_t)mr->render.camera.pos_x`, `s.face_z = (int16_t)mr->render.camera.pos_z` (beside the `memset` `:511`), and the drain write-back `e->row43 = r->row43` (beside `:1561`, where `face7d` already round-trips) | **NEW fixture** in `tests/test_engine_match_frame.c` (the `make_fixture`/`begin_isolated`/`one_granted_frame` frame-path harness), e.g. `test_mover_row43_live_turns_toward_focus` | new | with `e->row43 = 2` staged (via install 0x26), the record settled (target == pos → the focus branch, `move_attr >> 16 ≤ 0x60`, `action_handlers.c:221`), camera `(0,0,0)`, record `(0,0,0xB00)`, `face7d = 0x100`, `speed71 = 0`, no slot: today `s.direct_face` is memset 0 (`:511`) → `face7d` stays **0x100** (frozen), expected the first turn step **0x100 → 0x140** (limit `(0x10−0)<<2 = 0x40`, derived from `action_handlers.c:232-244`) and the octant cell `e->type` advanced from `fifa96_match_heading_table` | new |
| **P-TURN** the turn arm's live inputs | no production edit — coverage | extend **`test_step_direct_face`** (`tests/test_action_locomotion.c:118-149`) — **pre-existing pin reuse** (FU319 §4: target branch, focus branch, zero-delta keep, `direct_face==0` no-op all pinned) | none | **not a RED** (GREEN-today by construction): (a) the z-only focus arm (`face_x = 0, face_z = -1` → desired 0x200, facing steps to 0x40 — derived from `action_handlers.c:223-244`; the existing pin only exercises `face_x`); (b) the **native-value-2 equivalence** (`direct_face = 2` must produce the identical outputs to the `= 1` case at `:127-134` — the native byte is **2** (`row_flags & 0x02`, FU319 §2.2) and the consumer `0x8E29C` is a zero/nonzero test) | extended, GREEN-today |

**P-MOV riders** (same fixture, each a separate staged input, no camera/gate staging):

1. **row43 = 0 → no turn** — the mutation rider: a mover that ignores the latch fails it.
2. **The anti-derivation rider**: `e->row43 = 0` poked *after* an install of an armed row (e.g. 0x26), `anim_id` still 0x26 → the face must stay frozen. A mover (or dispatch staging) that derives `direct_face` from `anim_id`'s row flags — the spec §8.5/FU319 §5 rejected shortcut — turns the face and fails this rider.
3. **The clear-freeze rider**: run the P-CLR clear on the record, then a further frame with a fresh far target → the face stays frozen until a new install re-arms it.
4. **The far rider**: target far from pos (`> 0x60`) → the face steps toward `desired_facing` (the movement direction) — the explainer for the natural window's far-record turns (the replay's 4 140/67 320 record-frame collateral, FU319 §6, re-measured by FU323).
5. **The width rider** (optional, static): stage the camera beyond the word range → `(int16_t)` truncation is the native word read (`0x8E2B7 MOV CX,[EDX]` / `MOV AX,[EDX+8]`, FU319 §3.1). No divergence is reachable in the measured window (the camera stays within the OOB bounds); pin it as the width contract.

**Tripwires on the pins themselves.** (i) A P-TURN added assert that fails on the *current* tree contradicts FU319 §3's "instruction-for-instruction" claim — stop and report, do not fix the turn silently. (ii) The P-W3 pin must fail RED **on the value**, and the GREEN patch must not move any *observed* natural-pass cell before the first face-driven event — FU323's diff is the check (§5.3 TW1). (iii) The P-MOV fixture's staged distances must sit away from the `0x60` knife edge (e.g. 0 and > 0x60, never 96/97) so the P3 metric sub-change cannot flip the pin — the pins stay additive across P1→P3.

## 5. The golden recipe (verbatim, the P2 commit)

### 5.1 Ordering

P1 the latch + staging (§4) → **P2 the natural acceptance commit** (the inversion + P7/P8 + the golden re-pin + the provenance + the register pin, one commit — spec §8.6, FU321-L11) → P3 the metric sub-change (FU324's own pins and measured diff; never rides P2 — spec §8.2) → P4 the close-out register. The recipe below is P2's. FU322 owns the replacement-assert design; FU323 owns the measured values this recipe compares against.

### 5.2 The commands

```sh
# CWD = repo root; ISO present; the build green under ASan/UBSan (-Wall -Wextra -Werror)
./build/test_engine_m2 > /tmp/opencode/fu325/m2-new.txt         # the candidate transcript (stderr keeps the status)

# 1. the first differing byte/line (or let the test's own memcmp gate print it)
cmp tests/golden/engine/m2-frames.txt /tmp/opencode/fu325/m2-new.txt
#    expected fail shape: "…m2-frames.txt… differ: char B, line L"
sed -n "${L}p" tests/golden/engine/m2-frames.txt                 # line L's golden frame= id

# 2. the frame diff (the register's artefact)
diff -u tests/golden/engine/m2-frames.txt /tmp/opencode/fu325/m2-new.txt > /tmp/opencode/fu325/m2.diff

# 3. only-hash-moved check — frame ids AND state suffixes unchanged
diff <(cut -d' ' -f1,3- tests/golden/engine/m2-frames.txt) \
     <(cut -d' ' -f1,3- /tmp/opencode/fu325/m2-new.txt)          # expected: empty (exit 0)
#    (the FU319 §6.2 state= form)
diff <(cut -d' ' -f3- tests/golden/engine/m2-frames.txt) \
     <(cut -d' ' -f3- /tmp/opencode/fu325/m2-new.txt)            # expected: empty
grep -c ' state=' tests/golden/engine/m2-frames.txt /tmp/opencode/fu325/m2-new.txt   # the stratum counts: equal

# 4. the moved-line count (the house form's M)
diff -u tests/golden/engine/m2-frames.txt /tmp/opencode/fu325/m2-new.txt | grep -c '^+frame='

# 5. M1 immutable
./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt  # expected: silent, exit 0

# 6. the v21 additivity spine (the manual twin of the in-test gate, tests/test_engine_m2.c:6431-6451)
head -c 112487 /tmp/opencode/fu325/m2-new.txt | cmp - tests/golden/engine/m2-frames.txt
#    112487 B = the v21 2 522-line prefix; expected: silent

# 7. only after every gate above passes the analysis:
./build/test_engine_m2 > tests/golden/engine/m2-frames.txt       # the re-pin redirect (the documented form, :31)
sha256sum tests/golden/engine/m1-frames.txt tests/golden/engine/m2-frames.txt
make check
```

The in-test gates run the same checks automatically: the additivity gate (`:6431-6451`), the whole-transcript `memcmp` + `report_diff` (`:6452-6460`; the report prints `transcript differs at line N (byte B)` with the golden and actual lines — the first differing frame's identity comes from that line's `frame=` id, `report_diff` at `:5950-5966`). No `cmp` claim is valid without the ISO (the no-ISO mode skips the comparison, `:6409-6414`).

### 5.3 The stop-and-report tripwires

| # | tripwire | command + expected fail shape | action |
|---|---|---|---|
| **TW1** | the first differing frame is **earlier** than FU323's measured first face-driven event, or differs from it | the `cmp` line L → the `frame=` id; compare with the measured value (FU323 Step 2/4). Fail shape: an id < the measured first face-driven frame, or an id inside the m 41/m 42 staged band that the m-42 gate's own one-frame shift (FU321 §5.4 item 1) cannot explain | **STOP** — spec §7; a numbered leg, never a re-pin |
| **TW2** | a `state=` suffix moved (a phase/score stratum) | the step-3 cut-diffs non-empty, or the two `grep -c ' state='` counts differ | **STOP** — the producer must not move a phase/score stratum; FU323's `state=` check defines the expected-empty shape |
| **TW3** | a `frame=` id changed or a stratum added/removed (a line-shape change) | the step-3 `-f1,3-` diff non-empty | **STOP** — the face producer moves only `hash=` columns |
| **TW4** | M1 moved | the step-5 `cmp` prints anything | **STOP and report** — M1 `09b726b7…` is immutable (AGENTS.md); a move is a regression report, never an upgrade |
| **TW5** | the v21 additivity spine breaks | the step-6 `cmp` prints `differ: char …, line …` | **STOP** unless the provenance note's anchors field argues the crossing (it cannot for the face producer alone — argue only with FU323's measured cell evidence); unargued = STOP |
| **TW6** | a moved frame no face-gate reader can explain | the m2.diff's moved lines classified against the FU319 §5 lane list (the row-04 angle gate `0x7EE5C` + the defender decision, the row-0x0B clamp, the row-0x25 fold, the row-0x1B/0x16/0x24 re-faces, the impulsion arms via `+0x8E`, the pan's own camera/place motion) | **STOP** — spec §7: "moves frames no face-gate reader can explain" |
| **TW7** | the metric sub-change rode this build | `grep -n 'arm_dist_stage' src/fifa96_engine/fifa96_match_run.c` — `:521-522` must be unchanged in P1/P2; a golden measured with the metric swap applied is invalid | **STOP** — P3 is its own commit (FU324) with its own diff |

**The house-form provenance note** (the P2 commit, in `tests/test_engine_m2.c`'s provenance block — the phase-30 quoted precedent at `:1697-1707` is the form; FU321 §5.2 the axes): *"first differing line frame N, M hash lines, `state=` suffixes unchanged"* plus: old → new sha256 (both full 64-hex) + byte and line counts; the first/last differing line and its `frame=` id; the contiguous `frame=` range moved; the moved columns (expected `hash=` only); the `state=` strata (expected none touched/added/removed); the anchors crossed (m 41 / m 42 (`M2_M_BAND`) / m 58 (`M2_M_LINK`) / the v21 2 522-line prefix); the intended consequence (the first face-driven event = the measured first differing frame; the lane/arm classification per moved frame); the collateral (every other moved frame classified, or "none"); the phase-32 witness verdicts (per FU323's measured cells). The register pin lands in `docs/ENGINE.md` in the same commit; `docs/PROGRESS.md` updates when the slice closes (AGENTS.md).

**Current pins (fresh `sha256sum`/`wc` this session, §11):** M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (22 004 B, 691 lines); M2 `8434cfab105b926e78279ca3ea03c62bd3a0ae9654d2e11f0930c0cb8605cee1` (295 087 B, 6 522 lines) — matching the register (`docs/ENGINE.md:6155-6156`).

## 6. The Layer-2 live design

### 6.1 The vehicle: the natural run — no driven variant required

**Decision: Layer-2 runs the natural game run; a driven variant is NOT part of the GO.** Reasoning, from the carried record:

1. The phase-31 T3 drive and the phase-32 FU320 drive both exist **because the face was frozen**. FU314 §5 measured the undriven tape's only band entry (entity 12 from the east, 2 081 dispatch-side band frames) failing the angle gate at Δ 0x200 > 0xAB on every band frame — the face was the step-12 row-0x24 constant 0x100 and nothing re-faced it. The drives manufactured entries (the zero-delta point for T1, the west-cone offset tuple for FU320) where the frozen face folded to a pass.
2. Under the live face the **same undriven entry passes**: FU319 §6's open-loop replay puts the band record at face `0x300` (the camera park bearing) with Δ = 0 on 2 082/2 082 band frames, taking the focus branch on all of them (target distance `4..64 ≤ 0x60`). FU320-L3 already predicted the robust route ("the east approach passes at the first band frame with the walk-bearing face"); the live face realizes it on the undriven tape. Spec §8.1 fixes the vehicle accordingly: the undriven natural tape, "no added holds".
3. The Layer-1 numbers (the face at the band, the band count, the pan from the first band frame) are FU319 §6's *predictions* until **FU323** re-measures them on the patched probe — spec §6 makes that binding ("re-measured by the wave probe before the Stage-2 plan binds them").

**The fallback (and its trigger).** If FU323's natural measurement does not show the pan (the two realistic fail shapes: a **zero-seed silent event** — the no-slot arm's seed is the band record's velocity words, and `|w| ≤ 2` gives `v0 = 0` with `trk` bound and no motion, FU321 §3.5/§6.8; or the band entry's seed/gate combination measured differently than predicted), then: (a) the natural Layer-2 clause is **NOT-GO as measured** — record it as a leg with the native owner, do not tune the fixture; (b) the driven variant becomes the Layer-2 witness, replayed exactly as the phase-32 procedure (the M2 prefix by trace tick + the FU320 tail scheduled on the **wall clock** at phase 2 + 20 ticks — the p32 note's timing limits 1-3); (c) the register wording names it *"the pan reproduced live under the drive"* and the natural route stays the open leg — never silently upgraded (spec §7).

### 6.2 The walkthrough and the trace cells

```sh
DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p33/trace.log make game
# ./build/fifa96 game/FIFAPCCD96.iso — SDL3, the 960x720 client at +66,+69
# walkthrough: intro RETURN → front-end BACKSPACE → RETURN (match);
# then replay the M2_KEYS prefix by trace tick (the tape's own keys, p31/p32 method:
# RIGHT 7…47, KICK 14/17, LEFT 47…140, the act-1 KICK 1790/1797) and add NOTHING.
# No clip encoding during the walkthrough (the p32 note's timing limit 1: an
# encoding load compresses a trace-tick-driven hold — schedule anything timed on
# the wall clock; here nothing timed exists beyond the prefix).
```

The pan is expected to fire **by itself** at the measured band entry (the undriven approach runs from step ≈5065 on the null tape; the live tick drifts — the p31/p32 phase-2 landings were 1897/1800 vs the null's 1797/1802 — so every anchor is read from **this run's trace**, never assumed). The trace cells per line (`match_run.c:4591-4607`, the sink default-off; the header `:1849-1850`) and the expected shapes, following the p31/p32 note tables:

| trace line | expected cells | why |
|---|---|---|
| the gate frame | `pos` = the pre-pan place, `vel=0,0`, `spd=0`, `trk=-1`, `pan=0`, `ev=0x0`, `tim=0/0` | the camera still parked; the row-04 tuple is Layer-1, not a trace cell |
| the **event** frame | `pos` unchanged; `vel=(v0x,v0z)`; `spd=|v0|`; `trk=<the band record's id>`; `pan=0`; `tim=0/12`; `ramp=12` | `fifa96_camera_event_set`'s height-0 idle arm + the `0x71D27` tracked bind |
| the motion frames | `pos` monotone along the seed's sign; `vel` decaying `v0→…→1`; `pan` incrementing; `pos_y=0` | `camera.c:97-111/770-777` (carried FU321 §3.3) |
| the **stop** frame | `vel=0,0`, `spd=0`, `pan=n`, `pos` = the final `T(s)` | the `speed == 0` stop |

Reading traps (the FU321 §4.3 traps, restated for the live run): `ev` is `event_param` (the **height**), not the event code — the pan's signature is `trk` changing + `vel` jumping; `pan` counts pan **steps**, not frames. And two new ones for the natural run:

3. **Every changed-place run goes in the extract.** The natural tape continues past the first pan: the half-time roll reset the camera at `f=4141` in the phase-32 run (the p32 note's included non-cherry-pick); under the live face the second half re-places every record (the kickoff placement → row43 = 2 again) and later band entries may pan again. The extract must carry the whole run's changed-place windows (the p32 note included the half-time reset line — same rule), or the claim is a cherry-pick.
4. **The still control is camera-constant, not pixel-identical.** The phase-32 still pair was byte-identical because that drive's post-pan scene had settled to a single idle sprite. Under the natural tape the scene keeps animating (AI walk cycles), so `AE_still > 0` is possible and admissible; the still pair is a same-second, no-key, `pos`-constant (trace) pair, and **Gate 1 stays relative**: `AE_pan > AE_still`. `AE_pan ≤ AE_still` on a trace-constant still window is the fail signal.

### 6.3 The gates, the attribution rules, the capture stanzas

Gates (FU321 §4.6, mapped to the natural run): **G1 trace** — exactly the measured delta run(s), `trk` = the band record from the event through the run, `pan 0→n`, `pos_y=0`, `follow=0` on all lines; **G2 arithmetic** — the observed `(|Δ|, frames)` read off the trace must match `T(s)` from the **recorded** seed (the seed is the Layer-1 probe's recorded gate cell — the no-slot velocity arm `(word[+0x73]<<2, word[+0x75]<<2)` or the slot arm — never assumed `0x90`); **G5 pairs** — same-clock, both frames inside the delta run; **G6 AE** — relative, same session; **G7 bbox** — recorded, sprite-only, fail expected; **G8 invariants** — M1/M2 shas + `make check` + the census.

The attribution rules (FU321 §4.5, unchanged): the trace attributes, the AE corroborates; the trace and the captures are the same run; same-clock pairs only; the still control re-measured in-session, no absolute floor (FU309-L6); a band-limited bbox is entity motion, not a pan; **the offset/seed are Layer-1 citations** (the trace carries camera cells only — FU321-L1); no golden re-pin for a capture — a live path that moves M1/M2 stops the task for the written diff.

The capture stanzas (the registered p31/p32 forms; the p32 `import`+watcher route is primary, the p31 `ffmpeg` clip route the fallback for pair-hunting):

```sh
# the p32 route (primary): trace-bounded grabs from the same process
python3 /tmp/opencode/p33/p33-capture.py "$WID" /tmp/opencode/p33/trace.log /tmp/opencode/p33
# the p31 route (fallback): one clip, extract pairs inside ONE displayed second AND inside the delta run
DISPLAY=:1 ffmpeg -f x11grab -framerate 60 -video_size 960x720 -i :1+66,69 -t 70 /tmp/opencode/p33/p33-live.mkv

# the crops, the AE, the diff, the bbox (both pairs, plus the HUD clock crop)
for p in pan-a pan-b still-a still-b; do
  convert docs/screens/p33-l2-$p.png -crop 960x591+0+0 +repage /tmp/opencode/p33/c-$p.png
  convert docs/screens/p33-l2-$p.png -crop 140x66+0+650 +repage /tmp/opencode/p33/clock-$p.png
done
compare -metric AE /tmp/opencode/p33/c-pan-a.png /tmp/opencode/p33/c-pan-b.png null: 2>&1
compare -metric AE /tmp/opencode/p33/c-still-a.png /tmp/opencode/p33/c-still-b.png null: 2>&1
compare -metric AE /tmp/opencode/p33/clock-pan-a.png /tmp/opencode/p33/clock-pan-b.png null: 2>&1   # 0
compare -compose src /tmp/opencode/p33/c-pan-a.png /tmp/opencode/p33/c-pan-b.png \
  docs/screens/p33-l2-pan-diff.png
convert docs/screens/p33-l2-pan-diff.png -format '%@' info:      # the bbox (sprite-limited; recorded, never an attribution)
```

Artifacts (the p32 committed set): `docs/screens/p33-l2-pan-a.png`, `-pan-b.png`, `-still-a.png`, `-still-b.png`, `-pan-diff.png`, the trace extract `docs/screens/p33-l2-trace-pan.txt` (the phase-2 line, every changed-place window, the still window), and `docs/screens/p33-l2-notes.md` in the p31/p32 format (the run, the trace table, the AE table, the verdict, the timing limits found, the invariants).

### 6.4 The Layer-1 clause it cites

The natural window's Layer-1 acceptance (the band record's settled face, the gate pass count, the first pan frame, the seed) is **FU323's measured table**; spec §6's clause "the camera leaves (0,0,0) from the first face-driven event onward" is checked in-test by FU322's replacement asserts (the `:3722-3724` inversion + P7/P8, one commit). The Layer-2 note cites those measured cells; it derives nothing.

## 7. The hazard list with tripwires

| # | hazard | carried authority | tripwire |
|---|---|---|---|
| **H1** | **The one-frame successor placement.** The port's `fifa96_match_run_record_anim_successor` runs from the scene stage (`match_run.c:1781`), after the frame body; the native `FUN_0008DF84` fires per-frame inside `FUN_0008E008`'s record machine. The `row43` write there **inherits** that placement — the same one `row44` already carries. | FU319-L4; `match_run.c:1780-1793` fresh | The P-W2 pin asserts the **value** parity only. A diff whose moved frames sit at row-wrap boundaries (the two wraps `0x14→0x7F`/`0x52→0x7E`, FU318-L7) that a face-gate reader cannot explain = **STOP**. Do not "fix" the placement in this slice. |
| **H2** | **The two `0x8E`-branch install routes' values.** The team placement's two native routes (`FUN_0008E5BC` `0x8E6AF CALL 0x8DF84`; `FUN_0008E910` twin `0x8EA08/0x8EA0C`) and the head's jump-table row substitution (`0x62`/`0x65`, both flags `0x07`) have **no `src/` site**; the port's single kickoff placement covers their value. | FU319 §5 W3 note, §2.2 consequence | The P-W3 pin asserts row43 = 2 on **every** placed record (both teams, active/inactive/kept-special). In the natural window a record with `row43 = 0` that the native would have armed (its face frozen where FU319 §6's replay turned it) = a missed install route = **STOP** + leg. The pins pin the **value** (2), never a site claim. |
| **H3** | **The m-42/fixture class.** The m-42 band's staged `face7d = 0` equals the pre-fix constant — it cannot detect a re-staging regression; the link fixture pins the pan itself (`pos_z 0xB28`, `vel_z 5`, `goal_armed 1`); the mover's face write is **next-frame** input, so the m-42 gate may shift one frame; the half-faithful shape (live latch, focus left `(0,0)`) makes far records turn while the m-42 fixture does not. | FU321 §5.4 items 1-3 (E21/E22); spec §8.4 | The patched probe runs the m-42/link scenario **first** (FU323); a link-assert failure is the fixture-class stop, not a golden re-pin. The P-MOV pin must stage **both** the latch and the focus in one change. The fixtures' staged cells stay declared producer inputs (no staging converted to a gate — FU321 §5.4 item 6). |
| **H4** | **The claim ordering.** The pan is upstream of the claim (gate f → event f+1 → motion f+2; the row-05 claim is the same event's install). The natural run may pan a record that is not the possession record → no claim. | FU321 §6.2, L9; FU315 (the claim optional) | The acceptance never attributes the pan to the claim and asserts a claim only where FU323's measured cells bind the record to one. The claim pass's `claim_f` pins are recorded-then-re-pinned cells, never assumed. |
| **H5** | **The controlled fallback (`is_ball_track`).** With `tracked == NONE` the port sets `is_ball_track = (id == controlled)`, refusing arm B for the controlled record. On the undriven tape it is benign (band record 12 ≠ controlled 1, FU319 §6). | FU321 §6.1; FU314-L4 (`:129`); `match_handlers.c:2675-2681` fresh | If FU323's natural pan binds the **controlled** record, the fallback is no longer benign — **STOP**, it is FU314-L4's leg (out of this phase; never fixed here). |
| **H6** | **The zero-seed silent event.** The no-slot arm's seed `(4·w73, 4·w75)` with `|w| ≤ 2` → `v0 = 0` → `trk` binds, no motion, no changed frame, AE still passes (the scene animates). | FU321 §3.5/§6.8, L2; the m-42 staged `vel73/75 = 0` is that seed shape (E21) | G4: the **recorded** seed must satisfy `|v0| ≥ 1` (the corrected absolute form — the FU321 review erratum 1) and G1 must show the delta run. A trace with `trk` set, `pan` 0, no run = the fail signal. FU323's Step 2 measures the natural seed. |
| **H7** | **The zero-delta knife edge inverts under a settled face.** At a settled record parked at the camera (the T1 shape) the turn's focus branch sees `(0,0)` deltas and **keeps** the face (`action_handlers.c:229-231`); the face stays the approach/settled value (≈0x300 from an east approach) while the zero-delta bearing folds to 0x80 → Δ = fold(0x80 − 0x300) = 0x180 > 0xAB — the T1 pass may **fail** and the pan may instead arm at the last nonzero-lane approach frame (Δ ≈ 0 — the robust route, FU320-L3's shape). | derived from `action_handlers.c:223-231` + `entity_update.c:90-92` (the 0x80 fold, FU321 E12) + FU320-L3; **the verdict is FU323's measurement** | Neither the T1 nor the offset pass's pins may be assumed to hold: FU323 rules each (survives / re-pins with measured cells / stops). A Stage-2 pin that asserts the zero-delta degeneracy survives, or a witness "upgraded" to robust without measurement, is the spec §7 violation. |
| **H8** | **Census/README invariants.** The census is `60/80 = 75%` (`docs/ENGINE.md:6157`); the README title is derived, never hand-tuned (AGENTS.md). The row43 staging wires **no** action row. | AGENTS.md; FU321 §5.5 | `grep` the census headline before/after: unchanged. A wired-count change without a fresh census re-derivation = **NO-GO**; a README recompute without a fresh census = forbidden. |
| **H9** | **The honest-stop rule itself.** Any measurement contradicting the frozen prediction (FU319 §6/FU321 §5) becomes a numbered open leg with its native owner — never asserted, never explained away. | spec §7; FU319 §6.3 | Every tripwire above lands as a leg + a report; the producer plan halts (P2 blocked) until the controller re-scopes. The phase-32 witnesses' driven timelines may move — measure, rule, write the reason. |

## 8. Legs

1. **FU325-L1 — the natural Layer-2 pan is a prediction until FU323 measures it.** The acceptance vehicle is fixed (spec §8.1) but the pan's existence, size and first frame on the undriven tape are FU323's measured values; FU319 §6's open-loop numbers are inputs to that measurement, not acceptance numbers. Owner: FU323 → the Stage-2 P2 commit.
2. **FU325-L2 — the driven fallback's live timing.** If the fallback is invoked (§6.1), its own live↔null tick alignment must be re-measured (FU321-L4's general rule; the p32 note's wall-clock scheduling and the encoder-load collapse). Owner: the Stage-2 Layer-2 task.
3. **FU325-L3 — the kickoff narrow write's pool-row44 question.** The narrow write's required content is `row43`; whether it also writes pool `row44` (the native install writes both bytes) is an implementation decision whose blast radius is unproven — the pool `row44` consumer census between the placement and the first dispatch is not taken, so any pre-face-event diff that a `row44`-at-placement explains is a **stop** (TW1), not a re-pin. Owner: the Stage-2 P1 patch + FU323's diff.
4. **FU325-L4 — the successor placement (FU319-L4) is carried, not re-derived.** The row43 write inherits `row44`'s one-frame scene-stage placement; the native per-frame ordering stays the FU-84 driver's leg. Owner: FU319-L4 (unchanged).
5. **FU325-L5 — the natural run's later windows.** Under the live face the second half (the re-placement) and any later band entries can pan again; the natural acceptance's extract and the P2 golden accounting must cover the whole 12 000-step window, not the first pan only. The measured later-window cells are FU323's. Owner: FU323 + the P2 accounting.
6. **FU325-L6 — the trace's run-relative `f` and the pair window.** The natural pan's displayed-second window depends on the measured seed (a `T(0x90)` pan is ~0.9 s ≈ 27 frames; smaller seeds shrink it, FU321-L7); the pair rules are the same, the numbers are per-run. Owner: the Layer-2 note.
7. **FU325-L7 — the metric sub-change has no acceptance here.** Beyond the separation rule (P3 its own commit; the P-MOV fixtures knife-edge-free; TW7), the metric's RED/GREEN, census and blast radius are FU324's; the composition check (latch-then-metric additivity) is FU324 Step 3's. Owner: FU324.

## 9. GO/NO-GO

**GO on the acceptance design as specified** — the six-pin plan (§4), the verbatim recipe + seven tripwires (§5), the natural-vehicle Layer-2 (§6), the nine hazards (§7) — all of it implementable from the frozen authority without a new native port, without a golden prediction, and without a single number invented here.

**The Layer-2 GO is conditional on the natural measurement:** the natural run is the vehicle **iff** FU323's measured natural table shows the row-04 gate passing band frames with a non-zero seed (`|v0| ≥ 1`) and the first differing frame at or after the first face-driven event. Otherwise the natural clause is recorded as a leg and the driven FU320 fallback (the phase-32 procedure verbatim) is the Layer-2 witness — named as the fallback in the register, never silently substituted (spec §7).

**The P2 commit is atomic (spec §8.6, FU321-L11):** the `:3722-3724` inversion (fresh numbering; the spec's `:3701-3707` predates T1 — the phase-32 close-out erratum 3), FU322's P7/P8 replacement asserts, the golden re-pin, and the node's provenance note — one commit. Deleting or weakening the zero-camera asserts without the replacement window is **NO-GO**.

**NO-GO on:** (a) any acceptance number argued from this doc (circular evidence — the plan's Review Focus 5); (b) a golden re-pin without the frame diff + the house-form note + the register pin in the same commit; (c) the metric sub-change riding the face producer's commit (spec §8.2); (d) a staging converted into a camera/face gate, or the `direct_face`-from-`anim_id` derivation (spec §8.5); (e) silently fixing any FU319-L1/L2/L4/L5 leg; (f) a README/census change without a fresh census; (g) a Layer-2 attribution from the AE or the bbox alone.

## 10. Known limits

1. **No measurement in this slice.** Every quantity is either a first-hand `file:line`, a carried owner-cite, or a forward-cite to FU322/FU323/FU324. The doc's only derived numbers are the unit-pin oracle values (the turn steps, the RED values), each derived from the cited port lines and confirmed by the executor at RED/GREEN time.
2. **The two-step RED** (declare → RED → write → GREEN) is this doc's discipline, not yet a repo convention; the executor records each RED's failing pair in the task report.
3. **The Layer-2 design inherits the p31/p32 timing lessons** (the phase-2 landing drift, run-relative `f`, the trace-burst scheduling) — they are recorded limits there, re-measured per run here (FU325-L2/L6).
4. **The natural run's later windows** (post-half-time re-placement) are unmeasured; the extract covers them wholesale (FU325-L5) but their classification is FU323's.
5. **The `row44`-at-placement question** (FU325-L3) is unresolved by design — this doc chooses the conservative tripwire instead of asserting either shape.
6. **M1/M2 shas** are fresh this session but the Stage-2 plan re-verifies them before P2 (the pins can move between the wave and Stage 2 — never assumed stable).

## 11. Provenance and self-audit

**Fresh this session (repo reads only; no writes, no git, no Ghidra, no probe):** `AGENTS.md`; the spec §1-§9; the phase-33 plan `:1-116`; `docs/ghidra/FU319_r2l6_direct_face.md` (whole), `FU321_r2l6_nonzero_acceptance.md` (whole incl. the review errata), `FU314_r2l6_pan_triggers.md` (whole incl. the errata); `docs/screens/p31-l2-notes.md`, `p32-l2-notes.md` (whole); `docs/ENGINE.md` (`:5025-5064`, `:6095-6160`, `:6370-6519`); `src/fifa96_engine/fifa96_match_run.c` (`:480-599`, `:995-1094`, `:1330-1419`, `:1495-1574`, `:1665-1710`, `:1760-1804`); `src/fifa96_engine/fifa96_match_entities.c` (`:520-599`); `src/fifa96_engine/fifa96_match_handlers.c` (`:2598-2672`, `:5185-5234`); `src/fifa96_loader/fifa96_action_handlers.c` (`:190-309`); `include/fifa96_loader/fifa96_action_handlers.h` (`:118-157`); `include/fifa96_engine/fifa96_match_entities.h` (`:90-119`); `include/fifa96_engine/fifa96_match_run.h` (`:480-519`); `tests/test_action_locomotion.c` (`:1-200`); `tests/test_engine_match_frame.c` (`:3041-3115`, `:7560-7779`); `tests/test_engine_match_entities.c` (`:1055-1114`); `tests/test_engine_match_handlers.c` (`:4050-4139`); `tests/test_engine_m2.c` (`:1685-1794`, `:1860-1890`, `:2280-2319`, `:2430-2499`, `:3440-3509`, `:3560-3739`, `:4031-4285` (read `:4117-4196`), `:4285-4584`, `:5950-6019`, `:6020-6139`, `:6400-6467`); `CMakeLists.txt` (`:282`, `:540`, `:586`, `:594`, `:626`, `:631`); the goldens (`sha256sum` + `wc`): M1 `09b726b7…a2cd` (22 004 B, 691 lines), M2 `8434cfab…ce1` (295 087 B, 6 522 lines).

**Carried (owner docs named, never re-derived):** the producer census and write-site bytes (FU319 §2); the turn's instruction map and focus-pointer default (FU319 §3/§3.1); the minimal staging shape (FU319 §5); the open-loop prediction quantities (FU319 §6 — FU323 re-measures); the re-pin procedure and house form (FU321 §5.2/§5.3); the fixture hazards E21/E22 and the half-faithful shape (FU321 §5.4); the gate hazards E17, the claim ordering, the one-shot, the zero-seed event, the knife edge (FU321 §6); the closed FU314-L5 defect and the robust-route clause (FU314 L5 `:129`, the review erratum's stage-2 consequence `:174`); FU-185-L3 (the `0x8540C` clear, the missing cell); FU318-L7 (the alternates/wraps) as recorded in the phase-32 register (`docs/ENGINE.md:6486-6505`).

**Self-audit:** every port-side line cited above was read this session; every native byte/fact is named with its owner doc and section — none re-derived, none invented; the two derived oracle values (the 0x40 turn limit's first step; the 0x180 fold at a settled face) are derived from cited port lines and labeled as executor-confirmed at pin time; the three siblings' quantities appear only as "the measured value (FU322/FU323/FU324)" with no number attached; the RED states are stated against the staged-zero tree as the task asked; the doc creates one repo file (this draft), runs no git command, writes nothing to `build/`, `tests/golden/` or the ISO. No prior doc section is contradicted; where a line number moved (the baseline asserts `:3701-3707` → `:3722-3724`; the row-04 staging `:2646` → `:2654`), the fresh cite is given with the erratum that recorded the move (`docs/ENGINE.md:6480-6483`, the phase-31 close-out's stale-cite minors `:6145-6147`).

---

## Review erratum (wave review, 2026-10-22)

**1. TW1's fail shape was written before the measurement; the measured 852-chain redefines it.** Quote (§5.3 TW1): "the first differing frame is **earlier** than FU323's measured first face-driven event, or differs from it … Fail shape: an id < the measured first face-driven frame, or an id inside the m 41/m 42 staged band that the m-42 gate's own one-frame shift (FU321 §5.4 item 1) cannot explain". **Measured (FU323 §6/§7, closing FU319-L7):** the first GOLDEN diff is frame **852** (`cmp`: byte 37436, line 852) — not a face-driven event frame in any direct sense: the faces settle live at engine step 9, the first face-driven position movement is frame 52 (t0#6, the mover's stride-error face term), the camera cells never differ undriven, and 852 is the sit-1 half-time kickoff's camera reset to `(0,0,0)` revealing the since-372–379 face-driven t1 walk-in sprites at shifted pixels; "852 ≥ 52 ≥ 9" is how the spec §6 golden rule holds, not a same-frame identity. **Corrected tripwire:** the recipe trips on (a) a first differing frame that FU323's own patched build does not reproduce — measured: exactly line 852 on the same FU319 §5 patch, so any other id is STOP; (b) any moved diff whose `state=` suffixes change (TW2/TW3 carry the expected-empty shapes); (c) any frames before 852 differing — and **not** on a pre-pan camera expectation, because there is no undriven pan (the camera place is face-independent in this diff) and the spec's "at or after the first face-driven event" anchor is the face event, not a pan frame. The m-42/m-41 clause of the old fail shape remains only as the secondary check it already is.

**2. The Layer-2 vehicle decision is governed by FU323's measurement: the natural clause is NOT-GO as measured and the driven variant is the Layer-2 witness.** Quotes (§2): "Layer-2 (§6) runs the **natural game run — no driven variant is required** under the live face"; (§6.1, the Decision): "Layer-2 runs the natural game run; a driven variant is NOT part of the GO"; (§6.1 item 2): "the live face realizes it on the undriven tape"; (§6.2): "The pan is expected to fire **by itself** at the measured band entry". **Measured (FU323 §2/§4, leg FU323-L1):** the undriven patched tape's band entry (entity 12, step 5062) passes the row-04 gate once and `fifa96_camera_event_set` binds `trk=12` at granted f=1517 (tick 5060) with the **no-slot arm's** zero seed (`word[+0x73/+0x75] << 2` = ±4 → `v0 = 0`); no granted frame has `pos ≠ (0,0,0)` through f=3597 — the pan does **not** fire undriven. That is exactly §6.1's own first fail shape (the zero-seed silent event), so §6.1's fallback (a)–(c) applies as written, citing this measurement: (a) the natural Layer-2 clause is recorded NOT-GO-as-measured as a leg with its native owner (the seed-switch census, `outfield.c:685-701`) — no fixture tuning; (b) the driven variant is the Layer-2 witness, replayed per the phase-32 procedure and **re-pinned on FU323 §5's measured drive timelines** (the code-3 walk camera-event chain: claim pass gate f=1552, event 1553, motion 1554, stop 1581, 54 changed frames, `(−0x120,0,0)`; offset pass lane_at_pass **129** = bound 129, Δ=0, two segments); (c) the register wording names it "the pan reproduced live under the drive" and the natural route stays the open leg — never silently upgraded. The §6.2 trace table's event/motion/stop rows apply to the **driven** run; the undriven run's expected trace shape is the bind row (`trk=12`, `vel=0,0`, `spd=0`, `pan=0`, `pos=0,0,0` throughout — the FU321 §6.8 "almost right" trace, AE still passing). §9's own conditional ("the natural run is the vehicle **iff** FU323's measured natural table shows the row-04 gate passing band frames with a non-zero seed (`|v0| ≥ 1`) …") already encodes the rule — measured: the seed is zero, so the condition **fails** and the fallback governs; §9's NO-GO list is unchanged. The §6.4 sentence "spec §6's clause 'the camera leaves (0,0,0) from the first face-driven event onward' is checked in-test by FU322's replacement asserts" inherits FU322's erratum: the undriven measured consequence is the bind, not a place move, and the spec-level wording is the controller's erratum.
