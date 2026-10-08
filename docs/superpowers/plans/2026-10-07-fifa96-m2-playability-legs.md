# FIFA 96 M2 follow-up 2 — playability legs: outfield rows, live phase drivers, score events, live anim inputs: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the remaining recorded legs that stand between the current engine (wired rows 11/80, M2 tape forcing phases, entities not animated live) and a *playable* match: the two outfield row bodies (OL-70/OL-70a), the FU-143 phase drivers wired into the run loop, the goal/score event source (C3-OL2), and the live animation inputs (OL-80) plus kickoff placement. The M2 acceptance tape is extended to progress naturally (or records precisely what remains forced).

**Architecture:** Four phases. (1) Port rows 04/08 full records from their FU-142 Appendix J/K spans and wire them under the standing gate. (2) Wire the already-derived `fifa96_action_phase_*` drivers (FU-143) into `fifa96_match_run`'s frame body so periods progress on the selector-0 default path, replacing the tape's forced 0x13/0x14 with the derived sequence where the drivers reach it. (3) Derive and wire the `FUN_00093944`-family score-event writers (C3-OL2; 11 call sites censused in the T15 review) so the score changes from gameplay, not `add_goal`. (4) Stage live animation id/frame (native `0x36D44/0x36D4F`, pool `+0x28/+0x3D`) and kickoff placement into the render path (OL-80, OL-T11-9). Ghidra stays read-only; every RE claim appends to its owning FU doc as errata.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, Ghidra MCP (read-only) for RE; the null backend is the deterministic test host; SDL3 only for the optional `make game` smoke.

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md` (§3.4-3.5, §5, §6); scope authority `docs/ghidra/FU142_installer_arms_scope.md` (OL-70/OL-70a §6, App. J/K), `docs/ghidra/FU143_phase_rows.md`, `docs/ghidra/FU141_action_cluster_de.md` (OL-42/OL-80), `docs/ghidra/FU137_dispatch_mechanics.md`; predecessor plans `docs/superpowers/plans/2026-10-07-fifa96-m2-match.md`, `docs/superpowers/plans/2026-10-07-fifa96-m2-arms-and-wiring.md`.

## Global Constraints

- Baseline at plan start: `make check` **104/104**; wired rows **11/80** (`00,06,07,0F,18,1E,21,23,26,28,2A`); dispatch 68 UNSUP / 11 OK / 1 NOTF; tree at HEAD `a29fdb2` or later.
- C11, `-Wall -Wextra -Werror`; no warnings; `make check` green after every task.
- Engine tests build under ASan/UBSan.
- Evidence-gated: every RE claim cites bytes/addresses/tool output; unprovable → numbered open leg; corrections append as errata, never rewrite prior map rows. A task's exact new fields/constants are pinned by the FU appendix it writes from the first-hand window it cites.
- Authoritative program `/FIFA96.EXE` (pass explicitly; `fifa96.exe` collides with the loader). Flat offsets need `+0x100000`.
- Ghidra is read-only for implementers; all Ghidra writes and commits are serialized by the controller.
- A row is wired only when install arm + full record-visible body + pool binding are all bounded; otherwise `fn` stays NULL with a numbered open leg. Never a silent no-op success. The standing pushbacks (T6 row 29, T10 row 05, T14 rows 04/08) are the precedent: evidence beats the brief's expectation, and the deviation is documented.
- Known misread classes to guard: word pairs via `dword[addr]>>16` (the word is at `addr+2`), call targets (diff the CALL bytes, never infer from names), sign/width (reads and compares), inverted gates, missing conditional arms, stale request-field carry.
- Determinism contract: the null backend is the regression source of truth; every render/golden change re-pins its hashes in the same commit with a documented reason. The M1 golden must never move; if the M2 tape's hashes move, the change is either a deliberate progression upgrade (documented re-pin) or a regression (fix, don't re-pin).
- Commit style: `feat(...)`, `fix(...)`, `docs(fuNNN)`, `test(...)`, `chore(...)`.
- Original assets never committed; tests skip ISO-dependent cases when the ISO is absent.
- SDD tooling drives this plan; the controller keeps a ledger in `.superpowers/sdd/2026-10-07-fifa96-m2-playability-legs/progress.md` and updates it at each task boundary.

## File Structure

| Path | Responsibility |
|---|---|
| `src/fifa96_loader/fifa96_outfield.c` / `.h` | Row 04/08 record-visible bodies (extend; helpers per App. K) |
| `src/fifa96_engine/fifa96_match_handlers.c` (+ `.h` if the seam grows) | Row 04/08 binders + table classification/evidence (extend) |
| `src/fifa96_engine/fifa96_match_run.c` / `.h` | Phase-driver wiring into the frame body; live anim staging; kickoff placement (extend) |
| `src/fifa96_loader/fifa96_action_handlers.c` / `.h` | Score-event source helpers (C3-OL2) per owning cluster (extend) |
| `src/fifa96_engine/fifa96_match_entities.c` / `.h` | Pool `+0x28`/`+0x3D` (anim id/frame) + kickoff placement inputs (extend) |
| `tests/test_outfield.c`, `test_engine_match_handlers.c`, `test_engine_match_frame.c`, `test_engine_match_render.c`, `test_phase_drivers.c`, `test_engine_m2.c` + `tests/golden/engine/m2-frames.txt` | Expectation/chain/tape updates |
| `docs/ghidra/FU142_installer_arms_scope.md`, `FU143_phase_rows.md`, `FU141_action_cluster_de.md`, `FU137_dispatch_mechanics.md`, `docs/ENGINE.md`, `README.md` | RE record and status |
| `CMakeLists.txt` | New tests registration if any |

---

## Gate G1 — Outfield rows 04/08

### Task 1: OL-70 — row 04 full record-visible body and wiring

**Files:**
- Modify: `src/fifa96_loader/fifa96_outfield.c` / `.h`, `src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_04`), `tests/test_outfield.c`, `tests/test_engine_match_handlers.c`, `docs/ghidra/FU142_installer_arms_scope.md` (App. K.3/J errata), `docs/ghidra/FU141_action_cluster_de.md` / `FU75_outfield_decide.md` (errata), `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 04)
- Read: FU-142 §6 OL-70 + App. J/K (span `0x7E7C8..0x7F141`, RET at `0x7F141`; helper inventory)

**Interfaces:**
- Produces: `fifa96_err_t fifa96_outfield_row04_step(struct fifa96_outfield_record_view *rec, ...);` — the row-04 record-visible body per App. K.3 (fields/constants pinned there from the first-hand window).
- Produces (engine): `static int fifa96_match_action_04(struct fifa96_match_run *mr);` — binds the FU-141 input-row/forced-decision/chase arms to the row-04 step; row wired → `FIFA96_OK`.
- Consumes: Task 14's `fifa96_outfield_input_row`/`_chase_gate`, FU-141 pool, `fifa96_arm_face`/`fifa96_ball_fold` where App. K maps them.

- [ ] **Step 1: Failing test** — row-04 fixtures per branch (App. K.3 table); `action_expect[0x04]=FIFA96_OK` + dispatch test; the pin test for OL-70 is replaced by the run test.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU-142 App. K.3 + FU-137 class updates.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; pins re-pinned only with a documented reason.
- [ ] **Step 6: Commit** — `feat(engine): row 04 outfield body and wiring (OL-70)`.

### Task 2: OL-70a — row 08 full record-visible body and wiring

**Files:**
- Modify: `src/fifa96_loader/fifa96_outfield.c` / `.h`, `src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_08`), `tests/test_outfield.c`, `tests/test_engine_match_handlers.c`, `docs/ghidra/FU142_installer_arms_scope.md`, `docs/ghidra/FU141_action_cluster_de.md` / `FU75_outfield_decide.md`, `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 08)
- Read: FU-142 §6 OL-70a + App. K.4 (span `0x81068..0x814AF`, RET at `0x814AF`; the FU-141 `..0x81188` head is a prefix)

**Interfaces:**
- Produces: `fifa96_err_t fifa96_outfield_row08_step(struct fifa96_outfield_record_view *rec, ...);`
- Produces (engine): `static int fifa96_match_action_08(struct fifa96_match_run *mr);`; row wired → `FIFA96_OK`.
- Consumes: Task 1's patterns; FU-141 pool.

- [ ] **Step 1: Failing test** — row-08 fixtures per branch; `action_expect[0x08]=FIFA96_OK` + dispatch test.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; docs errata; FU-137 class updates.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`.
- [ ] **Step 6: Commit** — `feat(engine): row 08 outfield body and wiring (OL-70a)`.

**G1 gate:** whole-range review; rows 04/08 wired with full-body evidence; wired count 13/80.

---

## Gate G2 — Live progression and score events

### Task 3: Wire the FU-143 phase drivers into the run loop

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` / `.h`, `tests/test_engine_match_frame.c`, `tests/test_engine_m2.c`, `tests/golden/engine/m2-frames.txt` (only if the tape upgrades), `docs/ghidra/FU143_phase_rows.md` (integration errata), `docs/ENGINE.md`
- Read: `docs/ghidra/FU143_phase_rows.md` (all), child ledger records for C10 (selector-0 sequence 2 → 0x0C → 0; 0xC → 0x13 → 0x14 when `[0x157AC0]`)

**Interfaces:**
- Produces: `int fifa96_match_run_phase_drive(struct fifa96_match_run *mr);` — steps the derived `fifa96_action_phase_*` drivers from the run's phase/clock state each granted frame (the exact call shape per FU-143's derived entry points), with the class-1/class-2 gate and the `sec == limit+aux` period test driven by the match clock.
- Produces (behavior): the selector-0 default path reaches a live period end (2 → 0x0C → 0) without forcing `state.phase`; the M2 tape is upgraded to use the derived sequence iff the driver reproduces the forced behavior frame-for-frame, else the tape keeps its declared forcing and the limitation is recorded (with the evidence why) — **never re-pin the tape to hide a mismatch**.
- Consumes: FU-143 drivers (`fifa96_action_phase_row/_act/_situation/_period_end`), the FU-141 match clock/state.

- [ ] **Step 1: Failing test** — a frame-body test drives selector-0 to a period end via the driver (no forcing); a tape-mode test records whether the forced-phase tape still passes byte-identical.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU-143 integration errata; ENGINE.md update.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; if the tape moved, document the re-pin reason and diff the frames.
- [ ] **Step 6: Commit** — `feat(engine): live phase drivers in the match loop (FU-143 wiring)`.

### Task 4: C3-OL2 — goal/score event source

**Files:**
- Modify: `src/fifa96_loader/fifa96_action_handlers.c` / `.h` (or the owning module per the census), `src/fifa96_engine/fifa96_match_run.c` (score integration), `tests/test_action_handlers.c` (or owning suite), `tests/test_engine_match_frame.c`, `docs/ghidra/FU142_installer_arms_scope.md` (App. I.10 errata), `docs/ghidra/FU72_*.md` (errata), `docs/ENGINE.md`
- Read: FU-142 App. I.10 (the 11 `FUN_00093944` call sites `0x93D98..0x9486E`), FU-72 score/event model

**Interfaces:**
- Produces: `fifa96_err_t fifa96_match_score_event(...)` (or the derived writer shape) — the native score-event writer path used by the wired bodies; the engine's match state consumes it instead of (or in addition to) `add_goal` on the reached paths.
- Produces (behavior): a goal reached through a wired dispatch updates the score through the derived writer; `add_goal` remains only for paths whose writers stay unported (each a numbered leg).
- Consumes: T3's run loop, T1/T2's rows where the writers are reachable.

- [ ] **Step 1: Failing test** — the score event fires from a wired path fixture and updates state/score; the tape's goal step is re-checked against the new source (upgrade or explicit carry).
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; docs errata; ENGINE.md.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; tape/score assertions updated with reasons.
- [ ] **Step 6: Commit** — `feat(engine): goal score event source (C3-OL2)`.

**G2 gate:** whole-range review; live period end + score from gameplay on the default path; every remaining forcing is a numbered leg with evidence.

---

## Gate G3 — Live presentation

### Task 5: OL-80 + kickoff placement — live animation inputs and entity placement

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_entities.c` / `.h` (pool `+0x28`/`+0x3D`; kickoff placement), `src/fifa96_engine/fifa96_match_run.c` (staging into the bank chain), `tests/test_engine_match_entities.c`, `tests/test_engine_match_render.c`, `tests/golden/engine/m2-frames.txt` (re-pin iff entities draw), `docs/ghidra/FU141_action_cluster_de.md` (OL-42/OL-80 errata), `docs/ghidra/FU84_animation.md` (errata), `docs/ghidra/FU89_scene_assembly.md` (kickoff placement), `docs/ENGINE.md`
- Read: FU-141 §8 (OL-42/OL-80), FU-84 (bank row table `0x10EF00`), FU-89 (placement), FU-142 App. J

**Interfaces:**
- Produces: pool record fields for `anim_id` (`byte[[rec+0x28]]` source) and `frame` (`byte[rec+0x3D]`), staged into `fifa96_match_run`'s scene staging so `fifa96_arm_anim_select`/bank selection uses live values.
- Produces: kickoff placement (OL-T11-9) so at least the kickoff entity positions are non-zero at match start; the M2 tape re-pins iff the frames move (documented reason), and the render test gains a live-animation fixture.
- Consumes: T1-T4 state; FU-84 bank chain (C11).

- [ ] **Step 1: Failing test** — fixture entity's staged `anim_id`/`frame` vary across frames and the bank row follows; kickoff placement asserts non-zero positions.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; docs errata; ENGINE.md.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; tape re-pin only with the diff explained (drawing entities is an intended upgrade).
- [ ] **Step 6: Commit** — `feat(engine): live anim inputs and kickoff placement (OL-80/OL-T11-9)`.

**G3 gate:** whole-range review; entities animate and draw live on the tape.

---

## Gate G4 — Acceptance

### Task 6: Playability acceptance — tape v2, smoke, docs, final review

**Files:**
- Modify: `tests/test_engine_m2.c`, `tests/golden/engine/m2-frames.txt`, `docs/ENGINE.md`, `README.md`; no other code unless the smoke uncovers a defect.
- Read: spec §5; the T15 tape's provenance/checklist

**Interfaces:**
- Produces: the M2-B v2 tape — the same spec §5 sequence, but where Tasks 1–5 landed the natural path, the tape uses it (kick → score → period end without forcing); every remaining forcing is listed with its leg. Golden re-pinned with the documented reason and a frame-diff summary.
- Produces: the `make game` smoke re-run on this host with the reached-vs-blocked table updated; `ENGINE.md`/`README.md` status to the new wired count and legs.
- Produces: the plan's final whole-range review material (gates G1–G4, leg registry, re-pin ledger).

- [ ] **Step 1: Failing/updated test** — tape v2 assertions; stale golden fails where the sequence upgraded.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement** the tape/smoke/docs updates; re-pin the golden with the reason (`live progression/score/anim wiring upgraded the tape`).
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check` (ASan/UBSan); `make game` smoke result recorded.
- [ ] **Step 6: Commit** — `test(engine): playability acceptance tape v2 and docs`.

**G4 gate:** whole-range review; spec §5 sequence replay green; interactive smoke honest.

---

## Self-Review Notes

- **Coverage of the recorded legs:** OL-70/OL-70a → Tasks 1–2; live phase progression → Task 3; C3-OL2 → Task 4; OL-80/OL-T11-9 → Task 5; acceptance → Task 6. Not covered (deliberately): OL-48 rows 27/29/2C (entry-negative, static census closed), OL-62..69, OL-71, OL-72..79 (FU-143 loader legs not needed for progression), OL-81, palette install/HUD overlays (OL-T11 render legs beyond playability), C1-OL1/OL3.
- **Split rule (recursive, parent spec §12):** if a row's span or a task's scope exceeds a reviewer-sized unit, split at the FU boundary and record it in the ledger; rows 04/08 are already one task each.
- **Evidence-gate convention:** new symbol signatures are fixed here; each body's exact fields/constants are pinned by the FU appendix the task writes from the first-hand window it cites. No TBD/TODO/"similar to Task N".
- **Row discipline:** rows 04/08 flip only when their full bodies are tested (the standing gate); the phase/score/anim tasks change behavior only on paths whose evidence is first-hand, with jump-table re-pins documented.
- **Type consistency:** `fifa96_outfield_record_view`, `fifa96_match_run`, `fifa96_match_action_04/_08`, `fifa96_action_phase_*`, `fifa96_match_run_phase_drive`, pool `anim_id`/`frame` are used with the same meaning across tasks; the FU docs are the source of truth for fields/constants.

