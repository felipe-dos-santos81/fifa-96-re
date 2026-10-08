# FIFA 96 M2 follow-up 4 — playable match: pitch visibility, action rows, goal invokers: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the three carried blockers between the current engine and a self-playing, visible match: (1) the RGB pitch — install the match palette so the indexed draw from T1 (OL-T11-8) becomes visible in `make game` (OL-T11-6, the G1 carry); (2) the phase-1 record-action row machinery (rows 01/02 armed at `0x8D200`/`0x8D238`, plus rows 0x10..0x13) so the native kickoff chain reaches phase 2 on its own and the tape can eventually drop its 0x13/0x14/2 forcing (OL-84 residual, the G2 carry); (3) the goal invokers (`FUN_0008A938` situation 0xB producers and the goal-screen chain, OL-87/88/89) so goals arise from replicated gameplay rather than the derived score-source port alone.

**Architecture:** (1) palette: derive the native match palette setter/install chain first-hand (candidate leads in FU-96/FU-84/FU-137 vicinity; the null backend hashes palette planes — palette installs must remain deterministic and M1-safe), wire it into the match begin/frame path, verify RGB output via the smoke screenshots. (2) action rows: port the record-action row dispatcher/rows that produce the kickoff sequence (row 01 installs/executes via `0x7D9A4`, body `0x7DBC0` gate `phase==1`, situation 0xB at `0x7DF90` → `0x8AEF6` → `0x8AF02` → phase 2), then drop the M2 forcing **iff** the derived path reproduces the forced tape frame-for-frame; else keep forcing + mismatch evidence. (3) goal invokers: derive the situation-0xB producer(s) reachable from ported gameplay (row 01 first) and wire the score chain end-to-end. (4) acceptance v4 + smoke + docs.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, Ghidra MCP (read-only), SDL3 for `make game`; null backend = deterministic regression source of truth.

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md` (§3.4-3.5, §5, §6); scope authority: `docs/ghidra/FU143_phase_rows.md` (§8/§10 — corrected census `2f9957d`), `docs/ghidra/FU142_installer_arms_scope.md` (OL-T11-6/-7, App. I.10/L goal chain, App. J/K rows), `docs/ghidra/FU137_dispatch_mechanics.md` (§4 code 1 at `0x8D200`; §7 totals), `docs/ghidra/FU89_scene_assembly.md` (§11), `docs/ghidra/FU96_camera_objects.md`; predecessor plans: all four M2 plans incl. `2026-10-07-fifa96-m2-visible-match.md`.

## Global Constraints

- Baseline at plan start: `make check` **104/104**; wired **13/80** `{00,04,06,07,08,0F,18,1E,21,23,26,28,2A}`; dispatch 13 OK / 66 UNSUP / 1 NOTF; tree at `6cbe1bb` or later.
- C11, `-Wall -Wextra -Werror`; ASan/UBSan engine tests; ISO-gated tests skip without `game/FIFAPCCD96.iso`.
- Evidence-gated: first-hand bytes/addresses only; unprovable → numbered leg; errata append. Authoritative program `/FIFA96.EXE`; `+0x100000` rule; never objdump the stale flat bin; diff CALL bytes; word-pair/sign/gate traps.
- Ghidra read-only for implementers; commits + Ghidra writes serialized by the controller.
- Determinism: null backend source of truth. **M1 golden never moves.** M2 golden re-pins only for intended upgrades (palette visibility if it changes present hashes, forcing removal if frame-for-frame) with written reason + frame diff.
- Palette work must not break the null platform's palette hashing semantics; check `platform_null.c` conventions.
- Commit style: `feat/fix/docs(fuNNN)/test/chore`.
- Controller ledger: `.superpowers/sdd/2026-10-07-fifa96-m2-playable-match/progress.md`.

## File Structure

| Path | Responsibility |
|---|---|
| `src/fifa96_engine/fifa96_match_render.c` / `.h` | Palette install for the match canvas (extend) |
| `src/fifa96_engine/fifa96_match_run.c` / `.h` | Action-row dispatch wiring, begin/frame integration (extend) |
| `src/fifa96_engine/fifa96_match_handlers.c` / `.h` | Newly ported action rows (extend) |
| `src/fifa96_engine/fifa96_match_entities.c` / `.h` | Row-visible record fields if any (extend) |
| `tests/test_engine_match_render.c`, `test_engine_match_handlers.c`, `test_engine_match_frame.c`, `test_engine_m2.c` + `tests/golden/engine/m2-frames.txt` | Fixtures + tape v4 |
| `docs/ghidra/FU*.md` (new FU or appendices in FU-84/96/137/142/143), `docs/ENGINE.md`, `README.md` | RE record + status |
| `CMakeLists.txt` | Registration if new sources/tests |

---

## Gate G1 — RGB pitch visible

### Task 1: OL-T11-6 — match palette install

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_render.c` / `.h`, `tests/test_engine_match_render.c`, `tests/test_engine_m2.c` (palette assertions), `tests/golden/engine/m2-frames.txt` (only if present hashes move — expected if palette planes now change), `docs/ghidra/FU*.md`, `docs/ENGINE.md`
- Read: FU-96 (camera/slot; palette leads), FU-84 (HUD/overlays), FU-137 §7, T1's report §6.1 (RGB black diagnosis), `src/fifo96_engine/platform_null.c` palette hashing

**Interfaces:**
- Produces: `fifa96_err_t fifa96_match_render_palette_install(...)` (or the derived equivalent) — the native match palette setter applied to the canvas/palette state; deterministic under the null backend; RGB-visible under `make game`.
- Produces (behavior): the smoke screenshots show a non-black pitch (the drawn PLAYART/formation sprites + palette); M2 tape re-pins only if palette planes hash differently — with written reason + frame diff.
- Consumes: T1's formation draw (indexed), the null-backend present hash, the ISO resources if palette data is asset-loaded.

- [ ] **Step 1: Failing test** — palette assertion (install changes palette planes per the derived mapping; deterministic repeat) + smoke expectation (non-black). 
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU errata; ENGINE.md.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; golden decision with evidence; M1 must not move.
- [ ] **Step 6: Commit** — `feat(engine): match palette install — RGB pitch (OL-T11-6)`.

**G1 gate:** whole-range review; RGB visibility verified via smoke screenshots (non-black match canvas), indexed draw intact.

---

## Gate G2 — Action-row machinery → natural phase 2

### Task 2: OL-84 residual — phase-1 record-action rows

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c` / `.h`, `src/fifa96_engine/fifa96_match_run.c` / `.h`, `tests/test_engine_match_handlers.c`, `tests/test_engine_match_frame.c`, `tests/test_engine_m2.c` (+ golden iff forcing droppable), `docs/ghidra/FU143_phase_rows.md` (§8/§10), `docs/ENGINE.md`
- Read: FU-143 §10 corrected census (situation-0xB producers `0x7DF90` etc.; arm chain `0x8D1B1`/`0x8D200`/`0x8D238`), FU-137 §4 (code 1 at `0x8D200`), the row bodies `0x7DBC0` (gate `phase==1`), rows 0x10..0x13

**Interfaces:**
- Produces: the record-action row dispatcher + the rows needed for the kickoff sequence (at minimum row 01 → situation 0xB → `0x8AEF6` → `0x8AF02` phase 2; rows 0x10..0x13 as evidence dictates) with the derived arm/install semantics.
- Produces (behavior): a begun match reaches phase 2 naturally; the M2 tape drops the 0x13/0x14/2 forcing **iff frame-for-frame**; else forcing stays with exact mismatch evidence + leg update.
- Consumes: T2 (follow-up 3) phase-1 entry, FU-142a arm machinery, the lifecycle.

- [ ] **Step 1: Failing test** — frame-body test: begun match reaches phase 2 without forcing; tape-mode byte-identity probe if dropping forcing.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU-143 errata; ENGINE.md.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; golden re-pin only on the documented upgrade (frame diff) else forcing kept + mismatch evidence.
- [ ] **Step 6: Commit** — `feat(engine): phase-1 action rows — natural kickoff to phase 2 (OL-84)`.

**G2 gate:** whole-range review; natural kickoff → phase 2 (or forcing + full evidence recorded).

---

## Gate G3 — Goal invokers and score chain

### Task 3: OL-87/88/89 — goal invokers

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c` / `.h`, `src/fifa96_engine/fifa96_match_run.c`, `tests/test_engine_match_handlers.c`, `tests/test_engine_m2.c` (+ golden iff natural goals enter the tape), `docs/ghidra/FU142_installer_arms_scope.md` (App. I.10/L), `docs/ENGINE.md`
- Read: FU-142 App. I.10/L (the 11 `FUN_00093944` call sites `0x93D98..0x9486E` and their period-indexed handlers), T4 (follow-up 2) score-source port, Task 2's action rows (which row(s) are reachable from ported gameplay)

**Interfaces:**
- Produces: the derived goal-invoker path(s) reachable from the ported rows wired to `fifa96_match_run_score_event`; unreachable invokers stay documented legs with evidence.
- Produces (behavior): a natural score event in a headless fixture if reachable; tape asserts it iff natural.
- Consumes: T4's `score_event`, Task 2's rows, the period-indexed handler table.

- [ ] **Step 1: Failing test** — a fixture where the derived invoker increments the score via the wired source.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU-142 errata; ENGINE.md.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; golden decision with evidence.
- [ ] **Step 6: Commit** — `feat(engine): goal invokers and score chain (OL-87/88/89)`.

**G3 gate:** whole-range review; score from replicated gameplay (or legs with full evidence).

---

## Gate G4 — Acceptance

### Task 4: Playable-match acceptance — tape v4, smoke, docs, final review

**Files:**
- Modify: `tests/test_engine_m2.c`, `tests/golden/engine/m2-frames.txt` (only if moved by T1–T3 and not yet committed), `docs/ENGINE.md`, `README.md`, `docs/screens/*` (fresh smoke shots)
- Read: spec §5; the T3 (follow-up 3) tape provenance

**Interfaces:**
- Produces: tape v4 — the spec §5 sequence with palette visibility, natural phase-2 (if landed), natural score (if reachable); every remaining forcing listed with its leg.
- Produces: `make game` smoke re-run with RGB screenshots + reached-vs-blocked table.
- Produces: the plan's final whole-range review material (G1–G4, leg registry, re-pin ledger).

- [ ] **Step 1: Updated test** — tape v4 assertions.
- [ ] **Step 2: Run.**
- [ ] **Step 3: Implement** tape/smoke/docs updates.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; smoke recorded (non-black canvas iff T1 landed).
- [ ] **Step 6: Commit** — `test(engine): playable-match acceptance tape v4 and docs`.

**G4 gate:** whole-range review; tape v4 green; smoke honest.

---

## Self-Review Notes

- **Coverage:** OL-T11-6 → T1; OL-84 residual → T2; OL-87/88/89 → T3; acceptance → T4. Not covered (recorded legs for later): OL-83 (`type`/`actor_type`), OL-81 (ac5/ac7 carry), OL-82 (row-08 scan producer), OL-T11-7 (HUD), FU-96 leg 5 (per-record camera place `FUN_00079F3C`), formation-id producer (`[0x14C1E4]`/`[0x14C1E5]`), OL-85 (extra-time flag), OL-62..71 residuals.
- **Split rule:** if the action-row port (T2) exceeds a reviewer-sized unit, split at the dispatcher/row boundary (dispatcher + row 01 first) and record it in the ledger.
- **Evidence-gate convention:** new symbol signatures fixed here; fields/constants pinned by the FU appendix the task writes from its first-hand windows.
- **Determinism:** palette (T1) and forcing removal (T2) are the only expected M2 re-pins; each needs written reason + frame diff; M1 never moves.
- **Type consistency:** `fifa96_match_render_palette_install`, the row-dispatch seam, and `fifa96_match_run_score_event` keep their meanings across tasks; FU docs are the source of truth.

