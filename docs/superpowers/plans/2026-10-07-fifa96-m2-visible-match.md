# FIFA 96 M2 follow-up 3 — visible match: formation placement and kickoff entry: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the live match visible and self-progressing: land the formation/record placement that gives non-zero entity positions so players draw on the M2 tape and in `make game` (OL-T11-8), and derive the kickoff phase entry so the tape can drop its forced 0x13/0x14 phases (OL-84/OL-85). Both are the last recorded blockers between the current engine (13/80 wired, live driver, live anim inputs) and a playable, watchable match.

**Architecture:** (1) The formation data is resource-loaded: `FUN_0004A6BC` reads `t%s.dat`/`lay%s.fmt` through the resource pointer table `0x14BFC0` (BSS at EXE time; FU-89/FU-142 §6 OL-T11-8). The task derives the loader/placement chain first-hand, wires the derived formation targets into `fifa96_match_entities_place`/begin so records receive real positions, and re-pins the M2 golden as the intended drawing upgrade. (2) The kickoff entry (OL-79 static negative → OL-84) is derived where provable; if the derived entry reproduces the forced 0x13/0x14 behavior frame-for-frame the tape drops the forcing, else the forcing stays with the mismatch evidence recorded. (3) Acceptance v3 + smoke + docs.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, Ghidra MCP (read-only); the null backend is the deterministic host; SDL3 only for `make game`.

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md` (§3.4-3.5, §5, §6); scope authority `docs/ghidra/FU89_scene_assembly.md` (§11 errata; `0x79B6C` tail), `docs/ghidra/FU142_installer_arms_scope.md` (§6 OL-T11-8, Appendix J/K), `docs/ghidra/FU143_phase_rows.md` (§8 OL-79/OL-84/OL-85), `docs/ghidra/FU96_camera_objects.md`; predecessor plans `docs/superpowers/plans/2026-10-07-fifa96-m2-playability-legs.md` and the two earlier M2 plans.

## Global Constraints

- Baseline at plan start: `make check` **104/104**; wired rows **13/80** (`00,04,06,07,08,0F,18,1E,21,23,26,28,2A`); dispatch 13 OK / 66 UNSUP / 1 NOTF; tree at HEAD `809bfee` or later.
- C11, `-Wall -Wextra -Werror`; no warnings; `make check` green after every task; engine tests under ASan/UBSan.
- Evidence-gated: every RE claim cites bytes/addresses/tool output; unprovable → numbered open leg; corrections append as errata. The authoritative program is `/FIFA96.EXE` (flat `+0x100000`).
- Ghidra read-only for implementers; commits and Ghidra writes serialized by the controller.
- Known misread classes: word pairs via `dword[addr]>>16` (word at `addr+2`), call targets (diff CALL bytes), sign/width, inverted gates, missing conditionals, stale carry, gate-index misreads.
- Determinism: the null backend is the regression source of truth. M1 golden never moves. The M2 golden re-pins **only** for the intended drawing/entry upgrade, with a written reason and a frame-diff summary; never re-pin to hide a mismatch.
- Commit style: `feat(...)`, `fix(...)`, `docs(fuNNN)`, `test(...)`, `chore(...)`.
- Original assets never committed; ISO-dependent tests skip when the ISO is absent; resource files read from the ISO only.
- SDD tooling drives this plan; the controller keeps the ledger in `.superpowers/sdd/2026-10-07-fifa96-m2-visible-match/progress.md`.

## File Structure

| Path | Responsibility |
|---|---|
| `src/fifa96_loader/fifa96_scene.c` (+ `.h`) | Formation/resource load chain and derived placement inputs (extend) |
| `src/fifa96_loader/fifa96_iso9660.c` (read-only use) / resource plumbing | `t%s.dat`/`lay%s.fmt` reads through the derived table |
| `src/fifa96_engine/fifa96_match_entities.c` / `.h` | `kickoff_place`/`place` inputs to real positions; record target seeding (extend) |
| `src/fifa96_engine/fifa96_match_run.c` / `.h` | Begin wiring, phase-entry drive (extend) |
| `tests/test_scene.c`, `test_engine_match_entities.c`, `test_engine_match_render.c`, `test_engine_match_frame.c`, `test_engine_m2.c` + `tests/golden/engine/m2-frames.txt` | Fixtures + tape v3 |
| `docs/ghidra/FU89_scene_assembly.md`, `FU142_installer_arms_scope.md`, `FU143_phase_rows.md`, `docs/ENGINE.md`, `README.md` | RE record and status |
| `CMakeLists.txt` | Registration if new sources/tests |

---

## Gate G1 — Formation placement (visible match)

### Task 1: OL-T11-8 — formation/record placement from the resource tables

**Files:**
- Modify: `src/fifa96_loader/fifa96_scene.c` / `.h`, `src/fifa96_engine/fifa96_match_entities.c` / `.h`, `src/fifa96_engine/fifa96_match_run.c`, `tests/test_scene.c`, `tests/test_engine_match_entities.c`, `tests/test_engine_match_render.c`, `tests/test_engine_m2.c`, `tests/golden/engine/m2-frames.txt` (re-pin iff entities draw — the intended upgrade), `docs/ghidra/FU89_scene_assembly.md` (§11 errata), `docs/ghidra/FU142_installer_arms_scope.md` (OL-T11-8 status), `docs/ghidra/FU96_camera_objects.md` (if placement feeds the camera), `docs/ENGINE.md`
- Read: FU-89 §11 (`0x79B6C..0x79C1C`; `FUN_00073E08 → 0x8C24C → FUN_0008CF60(0x1588A4/0x1590D9)` 11-record loop), FU-142 §6 OL-T11-8 (`FUN_0004A6BC` reading `t%s.dat`/`lay%s.fmt` via `0x14BFC0`), FU-96 (camera ratio/slot data)

**Interfaces:**
- Produces: `fifa96_err_t fifa96_scene_formation_load(...)` — the derived formation/resource load (names/format/record layout per the first-hand chain; resource bytes from the ISO via the existing reader); returns `FIFA96_ERR_UNSUPPORTED`-style degradation when the ISO is absent (tests skip per house convention).
- Produces: placement input plumbing so `kickoff_place`/the begin path seeds each record's target/position from the formation data (the exact fields per FU-89 §11).
- Produces (behavior): at match start, player records have non-zero positions and pass the near-depth/lateral gates where the native would draw them; the M2 tape re-pins with the documented drawing-upgrade reason and a frame-diff summary (number of frames changed, what now draws).
- Consumes: T5's `0x79B6C` full port, the camera/slot data, the ISO reader.

- [ ] **Step 1: Failing test** — formation fixtures (loader: parse/placement with an ISO fixture if available; entity: non-zero seeded positions; render: an entity now draws) + the tape diff fails against the old golden.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU-89 §11 + FU-142 OL-T11-8 errata; ENGINE.md.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; re-pin `m2-frames.txt` with the reason and frame-diff summary; M1 must not move (if it does, stop and investigate).
- [ ] **Step 6: Commit** — `feat(engine): formation placement and visible entities (OL-T11-8)`.

**G1 gate:** whole-range review; entities draw on the tape and in `make game`.

---

## Gate G2 — Kickoff entry

### Task 2: OL-84/OL-85 — kickoff phase entry

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` / `.h`, `tests/test_engine_match_frame.c`, `tests/test_engine_m2.c` (+ golden iff the entry upgrades the tape), `docs/ghidra/FU143_phase_rows.md` (§8 OL-84/OL-85 status), `docs/ENGINE.md`
- Read: FU-143 §8.1 (OL-79 static negative; OL-84 kickoff entry; OL-85 extra-time flag), the phase machine (`fifa96_match_phase_machine`), T3's driver wiring

**Interfaces:**
- Produces: the derived kickoff entry (phase 2 entry or the 0x13/0x14 pair sequence per the evidence) wired into the run loop's kickoff path so the selector-0 default reaches phase 2 naturally.
- Produces (behavior): the M2 tape drops its forced 0x13/0x14 phases **iff** the derived entry reproduces the forced behavior frame-for-frame; otherwise the forcing stays, with the exact mismatch evidence (frames/state) and the leg updated — never re-pin to hide a mismatch.
- Consumes: T3's `fifa96_match_run_phase_drive`, the FU-142a arm machinery (0x26/0x28/0x2A staging), the lifecycle.

- [ ] **Step 1: Failing test** — frame-body test: a begun match reaches phase 2 (and the period-end path) without forcing; tape-mode check records byte-identity vs the forced tape.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU-143 §8 errata; ENGINE.md.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; golden re-pin only on the documented upgrade (frame diff), otherwise forcing kept + mismatch evidence.
- [ ] **Step 6: Commit** — `feat(engine): derived kickoff phase entry (OL-84/OL-85)`.

**G2 gate:** whole-range review; natural kickoff → phase 2 → period end on the default path (or the forcing + evidence recorded).

---

## Gate G3 — Acceptance

### Task 3: Visible-match acceptance — tape v3, smoke, docs, final review

**Files:**
- Modify: `tests/test_engine_m2.c`, `tests/golden/engine/m2-frames.txt` (only if not already re-pinned by T1/T2), `docs/ENGINE.md`, `README.md`; no other code unless the smoke uncovers a defect.
- Read: spec §5; the T6 v2 tape provenance

**Interfaces:**
- Produces: tape v3 — the spec §5 sequence with the drawing upgrade and (if landed) the natural kickoff entry; every remaining forcing listed with its leg (goal invokers OL-87/88/89, etc.).
- Produces: the `make game` smoke re-run with the reached-vs-blocked table updated (must now show drawn players/frames).
- Produces: the plan's final whole-range review material (G1–G3, leg registry, re-pin ledger).

- [ ] **Step 1: Updated test** — tape v3 assertions (drawn entities; entry path per T2's outcome).
- [ ] **Step 2: Run.**
- [ ] **Step 3: Implement** the tape/smoke/docs updates; golden re-pin iff it moved in T1/T2 and not yet committed with the current content.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check` (ASan/UBSan); `make game` smoke result recorded.
- [ ] **Step 6: Commit** — `test(engine): visible-match acceptance tape v3 and docs`.

**G3 gate:** whole-range review; tape v3 green; smoke honest.

---

## Self-Review Notes

- **Coverage:** OL-T11-8 → Task 1; OL-84/OL-85 → Task 2; acceptance → Task 3. Not covered (recorded legs for a later plan): OL-87/88/89 (goal invokers — score from gameplay), OL-83 (`type`/`actor_type` reconciliation), OL-81 (ac5/ac7 width carry), OL-82 (row-08 scan producer), OL-T11-6/-7 (palette/HUD), OL-62..71 residuals.
- **Split rule:** if Task 1's formation chain exceeds a reviewer-sized unit, split at the resource/placement boundary and record it in the ledger.
- **Evidence-gate convention:** new symbol signatures fixed here; fields/constants pinned by the FU appendix the task writes from its first-hand windows.
- **Determinism:** the M2 golden re-pin is expected only in T1 (drawing) and possibly T2 (entry); both need written reasons and frame diffs; M1 never moves.
- **Type consistency:** `fifa96_scene_formation_load`, `fifa96_match_entities_place` inputs, `fifa96_match_run_phase_drive` keep their meanings across tasks; FU docs are the source of truth.

