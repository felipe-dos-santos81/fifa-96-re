# FIFA 96 M2 follow-up 5 — interactive match: control consumers, camera place, HUD: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the visible, self-progressing match interactive and properly framed: (1) wire the control input to the derived locomotion/possession consumer so player input moves the controlled record (the recorded block: `input_state[0]` reaches the engine but no wired consumer exists — the unported possession/locomotion rows); (2) port the per-record camera place `FUN_00079F3C` (FU-96 leg 5) so both sides frame correctly instead of the positive-depth-only draw; (3) install the match HUD (OL-T11-7: score/clock overlay) so the score/period state is visible on screen.

**Architecture:** (1) control: derive which row(s)/handlers consume the pad for the controlled record in phase 1+ (the kickoff/possession chain: `entities.controlled`, `+0x826` side, the FU-73 possession invoker vicinity `0x7546E`/`0x75B58`/`0x76072` and the locomotion rows), port the reachable subset, wire `fifa96_match_input` → record velocity/target so movement is observable on tape and in `make game`; unreachable parts stay legs. (2) camera: port `FUN_00079F3C`'s per-record projection/place fields (the 0x15774C slot data) into the render/camera path. (3) HUD: derive the native score/clock overlay draw (FU-84/HUD leads, OL-T11-7) and render it on the indexed canvas + palette. (4) acceptance v5 + smoke + docs.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, Ghidra MCP (read-only), SDL3 for `make game`; null backend = deterministic regression source of truth.

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md` (§3.4-3.5, §5, §6); scope authority: `docs/ghidra/FU89_scene_assembly.md` §11 (`0x79B6C`/`0x79F3C` vicinity), `docs/ghidra/FU96_camera_objects.md` (leg 5 `:157-163`; camera/slot data), `docs/ghidra/FU142_installer_arms_scope.md` (OL-T11-6/-7; App. J/K), `docs/ghidra/FU143_phase_rows.md` (§8/§10/§11 corrected), `docs/ghidra/FU137_dispatch_mechanics.md` (§4/§6/§7), `docs/ghidra/FU144_match_palette_install.md`; predecessor plans: all five M2 plans incl. `2026-10-07-fifa96-m2-playable-match.md`.

## Global Constraints

- Baseline at plan start: `make check` **104/104**; wired **14/80** (`00,01,04,06,07,08,0F,18,1E,21,23,26,28,2A`); dispatch 14 OK / 65 UNSUP / 1 NOTF; tree at `e606acd` or later.
- C11, `-Wall -Wextra -Werror`; ASan/UBSan engine tests; ISO-gated tests skip without `game/FIFAPCCD96.iso`.
- Evidence-gated: first-hand bytes/addresses only; unprovable → numbered leg; errata append. Authoritative program `/FIFA96.EXE`; `+0x100000` rule; never objdump the stale flat bin; diff CALL bytes; word-pair/sign/width/gate/jump-table traps.
- Ghidra read-only for implementers; commits + Ghidra writes serialized by the controller.
- Determinism: null backend source of truth. **M1 golden never moves.** M2 golden re-pins only for intended behavior upgrades (movement, camera framing, HUD) with written reason + frame diff; never to hide a mismatch.
- Commit style: `feat/fix/docs(fuNNN)/test/chore`.
- Controller ledger: `.superpowers/sdd/2026-10-07-fifa96-m2-interactive-match/progress.md`.

## File Structure

| Path | Responsibility |
|---|---|
| `src/fifa96_engine/fifa96_match_input.c` / `.h` | Input mapping to record targets/velocity (extend) |
| `src/fifa96_engine/fifa96_match_handlers.c` / `.h` | Ported locomotion/possession rows (extend) |
| `src/fifa96_engine/fifa96_match_entities.c` / `.h` | Velocity/target fields (extend) |
| `src/fifa96_engine/fifa96_match_render.c` / `.h` | Camera place `FUN_00079F3C` fields, HUD overlay (extend) |
| `src/fifa96_engine/fifa96_match_run.c` / `.h` | Frame/begin integration (extend) |
| `tests/test_engine_match_handlers.c`, `test_engine_match_input.c`, `test_engine_match_render.c`, `test_engine_match_frame.c`, `test_engine_m2.c` + `tests/golden/engine/m2-frames.txt` | Fixtures + tape v5 |
| `docs/ghidra/FU89…/FU96…/FU84…/FU142/FU143/FU145 (HUD)`, `docs/ENGINE.md`, `README.md`, `docs/screens/*` | RE record + status |
| `CMakeLists.txt` | Registration if new sources/tests |

---

## Gate G1 — Interactive control

### Task 1: input → controlled-record locomotion

**Files:**
- Modify: the input/handlers/entities/run files above, `tests/test_engine_match_input.c`, `tests/test_engine_match_handlers.c`, `tests/test_engine_match_frame.c`, `tests/test_engine_m2.c` (+ golden iff movement enters the tape), `docs/ghidra/FU*.md`, `docs/ENGINE.md`
- Read: FU-137 §4/§6 (row semantics), FU-143 §10–11 (kickoff/possession chain `0x7546E`/`0x75B58`/`0x76072`; FU-73 vicinity), FU-142 App. J/K, the engine's `input_state`/`entities.controlled` seam, T4 f-up4 smoke finding ("keys reach `input_state[0]`, no wired consumer")

**Interfaces:**
- Produces: the derived control consumer — the row/handler that reads the pad for the controlled record in phase 1+ (locomotion/possession); wired so a headless fixture drives a record's target/velocity from input and the tape shows movement.
- Produces (behavior): observable movement on the M2 tape (and `make game` if the effect is visible with the camera fix); M2 re-pin iff the tape moves — with reason + frame diff.
- Consumes: `fifa96_match_input` mapping, `entities.controlled`, the arm machinery, T2 f-up4's row 01.

- [ ] **Step 1: Failing test** — input-driven target/velocity change on the controlled record; tape diff vs old golden.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU errata; ENGINE.md.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; golden decision (re-pin with reason + frame diff, or documented no-move).
- [ ] **Step 6: Commit** — `feat(engine): pad-driven locomotion for the controlled record`.

**G1 gate:** whole-range review; a headless input moves the controlled record; tape/behavior evidence.

---

## Gate G2 — Camera place and HUD

### Task 2: per-record camera place (`FUN_00079F3C`, FU-96 leg 5)

**Files:**
- Modify: render/camera sources + `tests/test_engine_match_render.c`, `tests/test_engine_m2.c` (+ golden iff framing changes), `docs/ghidra/FU96_camera_objects.md` (leg 5), `docs/ghidra/FU89_scene_assembly.md` §11, `docs/ENGINE.md`
- Read: FU-96 (`FUN_00079F3C` leads; `:157-163` leg; camera/slot fields), FU-89 §11 (`0x79F3C(EBX=0x15774C)` in the 11-record loop), T1 f-up3 report (positive-depth-only draw caveat)

**Interfaces:**
- Produces: the derived per-record camera place fields applied in the render path; both sides frame at kickoff (where the native does).
- Produces (behavior): tape re-pin iff framing changes (reason + frame diff).
- Consumes: the record pool, slot `0x15774C` semantics, the render projection.

- [ ] **Step 1: Failing test** — a record whose place field moves it into frame; render assertion (both sides draw at kickoff where native does).
- [ ] **Step 2: Run to fail.** 3. Implement + errata. 4. Run to pass.
- [ ] **Step 5: Gate** — `make check`; golden decision.
- [ ] **Step 6: Commit** — `feat(engine): per-record camera place (FU-96 leg 5)`.

### Task 3: match HUD (OL-T11-7)

**Files:**
- Modify: render + `tests/test_engine_match_render.c`, `tests/test_engine_m2.c` (+ golden iff HUD draws), new `docs/ghidra/FU145_match_hud.md` (or FU-84 appendix), `docs/ENGINE.md`, `docs/screens/*`
- Read: FU-84 (HUD/overlays), FU-142 OL-T11-7, FU-144 (palette conventions), the period/score state (`fifa96_match_run`), `FIRSTHAND` HUD draw chain to derive

**Interfaces:**
- Produces: the derived score/clock (and period) overlay draw on the indexed canvas + palette; deterministic null hash contribution.
- Produces (behavior): visible HUD in `make game`; tape re-pin iff HUD pixels enter the tape (reason + frame diff).
- Consumes: the render/palette path, the score/clock state.

- [ ] **Step 1: Failing test** — HUD pixels present when state set; absent/zero otherwise.
- [ ] **Step 2: Run to fail.** 3. Implement + errata. 4. Run to pass.
- [ ] **Step 5: Gate** — `make check`; golden decision.
- [ ] **Step 6: Commit** — `feat(engine): match HUD score/clock overlay (OL-T11-7)`.

**G2 gate:** whole-range review; camera framing + HUD visible with evidence.

---

## Gate G3 — Acceptance

### Task 4: Interactive-match acceptance — tape v5, smoke, docs, final review

**Files:**
- Modify: `tests/test_engine_m2.c`, `tests/golden/engine/m2-frames.txt` (only if moved and not yet committed), `docs/ENGINE.md`, `README.md`, `docs/screens/*`
- Read: spec §5; the T4 f-up4 tape provenance

**Interfaces:**
- Produces: tape v5 — the spec §5 sequence with input-driven movement, camera framing, HUD; every remaining forcing listed with its leg.
- Produces: `make game` smoke re-run with fresh screenshots + reached-vs-blocked table (movement observable, HUD visible, kick/score still leg-blocked).
- Produces: the plan's final whole-range review material (G1–G3, leg registry, re-pin ledger).

- [ ] **Step 1: Updated test** — tape v5 assertions.
- [ ] **Step 2: Run.** 3. Tape/smoke/docs. 4. Run to pass.
- [ ] **Step 5: Gate** — `make check`; smoke recorded.
- [ ] **Step 6: Commit** — `test(engine): interactive-match acceptance tape v5 and docs`.

**G3 gate:** whole-range review; tape v5 green; smoke honest.

---

## Self-Review Notes

- **Coverage:** control → T1; camera place → T2; HUD → T3; acceptance → T4. Not covered (recorded legs): OL-87/88/89 goal chain, OL-84a..h residuals, OL-T11-6 carried legs (`0x14B200`/translation/shade cube), formation-id producer, OL-83, OL-81/82, OL-62..71, extra-time flag OL-85.
- **Split rule:** if T1's locomotion chain exceeds a reviewer-sized unit, split at the input-mapping/row-port boundary (mapping + one movement row first) and record it in the ledger.
- **Evidence-gate convention:** new symbol signatures fixed here; fields/constants pinned by the FU appendix the task writes from its first-hand windows.
- **Determinism:** expected M2 re-pins: T1 (movement), T2 (framing), T3 (HUD) — each with written reason + frame diff; M1 never moves.
- **Type consistency:** the input→record seam, camera place, and HUD draw keep their meanings across tasks; FU docs are the source of truth.

