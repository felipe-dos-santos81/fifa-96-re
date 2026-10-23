# FIFA 96 M2 phase-35 producers — the placement timer and the natural pan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Land the native `+0x79` placement tier producer and the `+0x7B` row-09 correction (FU328-L1/L2) as one atomic, golden-moving fidelity commit, then land the **producer-conditioned natural no-slot pan acceptance** on the undriven tape, the Layer-2 live acceptance, and the close-out. M2 re-pins under the faithful-move policy (first differing line **852**, 5 671/6 522 lines, the 10-tag stratum set intact); M1 stays byte-identical.

**Architecture:** Four tasks: (1) the atomic producer commit (the cells + the producer + the row-09 OL-82 correction + the unit pins P1/P2/P3 + the A3 re-pin + the survival/driven re-pins + the M2 re-pin + the provenance + FU324-L5's trigger); (2) the natural witness (test-side only — cannot move M2); (3) the Layer-2 live acceptance; (4) the close-out register.

**Tech Stack:** C17/CTest, the null-backend harness, SDL3 `make game`, the env-gated camera trace, ffmpeg/ImageMagick.

**Spec:** `docs/superpowers/specs/2026-10-23-fifa96-m2-phase35-placement-timer-design.md` — read its **Plan erratum (wave close)** first (six corrections: the cap, the assumption, A3-in-P1, Branch A, the cite, the witness's producer-conditioned character).

## Decisions carried into this plan (the frozen wave, FU330–FU333 + errata)

- **The producer (FU330/FU331):** `+0x79` is a per-player tier lookup —
  `word[+0x79] = byte[0x10F394 + byte[[rec+4]+0xD]] | byte[+0x9D]`; the table
  is 32 bytes (`06×8 07×8 0F 0F 0F 0E 0D 0D 0C 0B 0B 0A 09 09 08 07 07 06`),
  **15 the in-range max**, **6 not a floor** (P2 reaches 3, 2 by composition);
  the index is unguarded; the roster per-record kickoff value is **unmeasured**
  (FU330-L1). The port's `player_d = 0` stages **6** on all records. The three
  native writers: P1 (`FUN_0007997C`, the prelude), P2 (`FUN_0008C758`, the
  rebind re-seed), P3 (the row-0x0C handler's `max(v>>1, 3)`); **only P1 is
  landed** (P2/P3 stay legs: the live domain 2–15, where 2 fails and 3 pans).
- **The port shape (FU331):** a shared 32-byte tier table — reuse
  `fifa96_arm_26_timer_table[32]` (the `0x20` boundary; `0x20–0x2F` is the
  `0x10F3B4` continuation — never zero a live cell); the cells `timer79` +
  the two declared staging bytes (`desc_d` = `[[rec+4]+0xD]`, `byte9d` =
  `+0x9D`); the writers: `reset_lane` (the native `0x79A4D→0x79A86` site, dead
  live — lands as the control) and `entities_init` (the live derived seam for
  the unported `0x741C4` driver); both write `+0x79` then `+0x7B`. Value today
  = `table[0] = 6` (declared staging, not native per-record).
- **The row-09 correction (FU331/FU332):** native `word[+0x7B] =
  word[+0x79]*2` (fresh `0x80B3D..0x80B43`); the port's literal 0 at
  `handlers.c:5034` (OL-82) becomes the product. At 6: `+0x7B = 12`,
  `vel73 = 12`, `speed71 = 12`, `target = pos + 96`; **inert** on the measured
  tape (0 code-9 record-frames — FU332; retire OL-82 for fidelity, zero blast
  radius). The `dword[+0x7B]>>16` reads are `word[+0x7D]`/`face7d` (the FU178
  alias trap — FU331-L5).
- **The measured M2 (FU332):** first differing line **852** (`state=1/1-0`),
  **5 671 / 6 522 lines** (86.95 %), one contiguous tail 852..6522, the 10-tag
  stratum set intact, `d5446144…` → **`6aa2703b…`** (6) / **`bd971062…`** (15);
  M1 `09b726b7…` byte-identical under the producer; the pool diverges at
  **step 12** (the first mover, id 10; all 20 active records by f=3; the
  render's 852-chain is the phase-33 attribution class).
- **The measured natural pan (FU332):** at 6 — entry f=703, **event f=704**
  (step 2355), entity 12, code 4, `has_slot 0`, lane 56, `vel73 −6`, seed
  `−0x18`, **`v0 −2`**, **`T_x −0x26` (−38)**, 13 frames (705..717, stop
  718), pan 2; at 15 — event f=607, lane 40, `v0 −4`, `T −0x84`, 27 frames,
  pan 4. The silent bind (f=1517) is **gone**; frequency 1 pan / 3 598.
  **The witness is producer-conditioned** (the native per-record value is
  unmeasured — the register must say so).
- **The acceptance (FU333):** the witness is **FU329 Branch A** (exists);
  C1–C6; the A3 window re-pins in **P1** (a green gate); the golden recipe:
  the first-diff anchor is this producer's own chain (the render 852), the
  `state=` check, the 10-tag stratum gate, the 6 522-line invariant, M1 cmp,
  the tripwires TW1–TW9; P2 is test-side and cannot move M2.

## Global Constraints

- `make check` green (strict `-Wall -Wextra -Werror`, ASan/UBSan; ~210 s →
  ≥600 s timeout) before every commit; **the producer commit is atomic** (the
  producer + row-09 + A3 + the survival/driven re-pins + the golden re-pin +
  the provenance + FU324-L5 — no red intermediate); work on `main`; commit and
  push per task.
- M1 `09b726b7…` immutable — `cmp` after every engine-touching task.
- M2 re-pins **only** in task 1's commit, with the written reason in the house
  form ("first differing line frame 852, 5 671 hash lines, `state=` suffixes
  unchanged"), the frame diff shown, the 10-tag stratum gate, the 6 522-line
  invariant; **tripwires:** any first-diff frame ≠ 852, any `state=` change,
  any moved frame FU332's build does not reproduce → STOP-and-report, never a
  re-pin.
- The producer matches FU330/FU331: the table reuse (`fifa96_arm_26_timer_table`,
  the `0x20` boundary, no zeroing fallback), the cells, the two writers, the
  values — **no native per-record claim** (the roster index is FU330-L1);
  **P2/P3 are not landed** (legs).
- The row-09 correction is FU331's `+0x7B = +0x79*2`; the OL-82 literal is
  retired at `handlers.c:5034` only.
- The census holds `60/80 = 75%` (a state-fidelity change, not an action row);
  the README is recomputed only if a fresh census says otherwise.
- The witness stages **no slot/seed/face/timer cell**; the FU329 Branch A
  rules bind (the source check + the tape memcmp + no new staging define).
- Docs append-only; the SDD ledger at `.superpowers/sdd/phase35/progress.md`
  (git-ignored); the ISO read-only; captures under `docs/screens/p35-*`.

## Review Focus

1. **The producer's identity** — the tier lookup (not a timer), the table reuse
   (the `0x20` boundary, no zeroing), the two writers, and the declared staging
   (the value 6 is not claimed native).
2. **The row-09 correction** — the product `+0x7B = +0x79*2` (the alias trap
   respected), the OL-82 retirement limited to the literal.
3. **The golden accounting** — the measured 852/5 671/10-tag facts, the
   tripwires, M1 cmp, the written reason; no silent re-pin.
4. **The witness's honesty** — producer-conditioned, no staged cell, `|v0| ≥ 1`,
   the measured tuple, the recorded/asserted split.
5. **The A3 re-pin in P1** — a green gate; the exact measured pacing; the
   phase-33 inversion's window semantics preserved.

---

### Task 1: The atomic producer commit

**Files:**
- Modify: `include/fifa96_engine/fifa96_match_entities.h` (the `desc_d`/`byte9d` staging bytes), `src/fifa96_engine/fifa96_match_entities.c` (the producer at `reset_lane` + `entities_init`), `src/fifa96_engine/fifa96_match_handlers.c` (the row-09 `+0x7B` correction at `:5034` + any needed read), `src/fifa96_engine/fifa96_match_run.c` (the init seam/pool staging if needed)
- Modify: `tests/test_engine_match_entities.c` (P1), `tests/test_engine_match_handlers.c` (P2/P3), `tests/test_engine_m2.c` (the A3 re-pin + the survival pins + the provenance), `tests/test_engine_match_frame.c` (the mover pin if the stride changes)
- Test: the focused binaries, then the full suite

**Interfaces:**
- Consumes: FU330/FU331 (the producer/table/cells), FU332 (the measured M2 + pool timeline), FU333 (the acceptance/golden recipe), FU324-L5 (the `+0x7B` nibble trigger).
- Produces: the live producer; the row-09 correction; the pins; the re-pinned A3 + survival set; the re-pinned M2 golden; the provenance. Tasks 2–4 cite these.

- [ ] **Step 1: Declare-first.** Add the cells (`desc_d`, `byte9d`) and the table access compile-only; green compile before any assert.
- [ ] **Step 2: RED the unit pins** (FU331's P1/P2/P3): P1 the producer (`init → timer79 == 6`, `timer7b == 6`; the index cases 22→0x0C, 16|0x10→0x1F, 0|0x80→0x86; RED: first assert 0); P2 the counter (`desc_d = 22 → install → timer7b == 0x0C`, nibble ≥ 3; RED: 0); P3 the row-09 consumer (face 0x100, speed 0 → `timer7b == 12`, `vel73 == 12`, `speed71 == 12`, `target_x == 0x160`; RED: 0/0/0/0x100). The known mover pin (`test_action_09_stage0_set_branch`'s `target_x`) must be re-derived (`vel<<3`).
- [ ] **Step 3: Implement the producer** (the two writers, the table reuse, the `0x20` boundary, no zeroing fallback) + **the row-09 correction** (`+0x7B = +0x79*2`; the OL-82 literal retired at `:5034` only).
- [ ] **Step 4: GREEN the unit pins.**
- [ ] **Step 5: The A3 re-pin** (in this commit — a green gate): the `tests/test_engine_m2.c:3972-3979` window constants with the measured pacing (the exact measured first-left-zero step; the alignment-only slack per FU333); the phase-33 inversion's semantics preserved.
- [ ] **Step 6: The survival/driven re-pins:** the driven claim/offset passes and the survival pins S1–S9 (FU333); the changed cells from FU332's measured tables; the drive cells where the producer moves them.
- [ ] **Step 7: The M2 re-pin** (the faithful-move): `./build/test_engine_m2 > /tmp/opencode/p35-m2.new`; `diff -u tests/golden/engine/m2-frames.txt <new>`; the first differing frame must be **852**; the moved set must match FU332 (5 671 lines, one contiguous tail 852..6522, the 10-tag stratum set intact, the 6 522-line invariant); M1 `cmp` byte-identical; re-pin the golden; the provenance reason in the house form; FU324-L5's re-measure clause recorded (the `+0x7B` nibble now moves).
- [ ] **Step 8: GREEN the suite.** `make check` 111/111; M1/M2 `cmp` (the new pins).
- [ ] **Step 9: Commit and push** `fix(fu328): the +0x79 placement tier producer (the natural pan's root fix)`.
- [ ] **Step 10: Ledger.**

### Task 2: The natural witness (test-side only)

**Files:**
- Modify: `tests/test_engine_m2.c` (the natural-pass witness per FU333 C1–C6)

**Interfaces:**
- Consumes: the measured tuple (FU332), FU333's C1–C6 + FU329 Branch A.
- Produces: the natural no-slot pan witness (the undriven tape); no golden change.

- [ ] **Step 1: The witness pins** (recorded-vs-asserted per FU333): the gate tuple at the entry/event (f=703/704 at 6), `has_slot 0`, the seed `(int16)(vel73<<2)`, `|v0| ≥ 1`, the event (`trk=id`, `tim 0/12`, `ramp 12`), the run (`segments ≥ 1`, the `T(s)` per axis from the recorded seed, the stop, the pan, `pos_y 0`), the frequency recorded; the no-staged-cell source check.
- [ ] **Step 2: GREEN** the focused test + the suite; M1/M2 `cmp` unchanged (test-side).
- [ ] **Step 3: Commit and push** `test(fu333): the natural no-slot pan witness (producer-conditioned)`.
- [ ] **Step 4: Ledger.**

### Task 3: The Layer-2 live acceptance

**Files:**
- Create: `docs/screens/p35-l2-notes.md` (+ the `p35-l2-*` artifacts per the p31–p33 formats)

**Interfaces:**
- Consumes: FU333's Layer-2 design; the witness drive (the undriven natural tape); the p33 capture method.
- Produces: the live acceptance record.

- [ ] **Step 1: Run the live undriven match** (`DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p35-cam-trace.log make game`; the phase-8 walkthrough; the natural tape, no added holds; the phase-2-relative anchors).
- [ ] **Step 2: Capture the pairs** (the pan/still pairs inside the displayed second and the trace delta run; the crop/AE/bbox recipe; the segment-aware anchors).
- [ ] **Step 3: Extract the trace** (the pan window: the event, the motion, the stop, `trk`, `pan`, `pos_y`, `follow`; the measured tuple vs the tree's pins).
- [ ] **Step 4: Write the note** with the honest verdict: **the natural pan reproduced live under the producer-conditioned witness** (the mechanism native-faithful; the value conditioned on the unmeasured roster index — say so); the timing limits/waiver option.
- [ ] **Step 5: Commit and push** `docs(fu333): the placement-timer live acceptance`.
- [ ] **Step 6: Invariants.**

### Task 4: The close-out register

**Files:**
- Modify: `docs/ENGINE.md` (the phase-35 register), `docs/PROGRESS.md` (the narrative), the phase-35 spec (the close-out erratum if any)
- Test: the final commands

**Interfaces:**
- Consumes: tasks 1–3; the frozen wave FU330–FU333.
- Produces: the frozen record.

- [ ] **Step 1: The register** (append-only): the verdict (the producer landed; the measured 852-chain re-pin; the **natural pan** — producer-conditioned; the row-09 correction; the P2/P3 legs); the wave verdicts + errata; the legs (FU330-L1..L8, FU331-L1..L8, FU332-L1..L7, FU333-L1..L15) with owners; FU328-L1's closure and the **P2/P3 residual** (the live domain 2–15; 2 fails, 3 pans); the roster-index leg (FU330-L1) as the native-faithfulness owner; FU324-L5's re-measure outcome.
- [ ] **Step 2: The PROGRESS narrative** (the natural pan's honest shape; the next candidates: P2/P3, the roster index, the claim/carrier fusion, the armer/follow, the half-time stall).
- [ ] **Step 3: The invariants** (the fresh census `60/80`; `make check` 111/111; M1/M2 `cmp`; the README).
- [ ] **Step 4: Commit and push** `docs(fu330): the phase-35 close-out — the placement timer and the natural pan`.
- [ ] **Step 5: Final ledger + report.**
