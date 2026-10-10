# FIFA 96 M2 phase-9 — live pad loop: on-screen movement, live carrier, pan origin, tape goal

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Serialized tasks.

**Goal:** Make the live surface physically playable and let the natural goal reach the tape: (1) on-screen movement — port FU-75 L4.1/L4.2 (forced-decision/chase application) + L4.6 (row 02 + keeper tables) so a held key visibly moves the controlled record; (2) live carrier producers so the wired pad kick has a ball to kick; (3) the remaining camera-pan producers (the nine `FUN_00071C94` callers, `walk_gate`, the half-line band reach, `FUN_00071DF4`) so the camera pans naturally — which arms set pieces/taker rows and produces the first natural tape goal (**expected M2 re-pin: first since S1**); (4) acceptance v9 with visible movement/kick smoke and the natural goal pinned.

**Architecture:** Tasks port from the named FU legs (FU-75 L4.x, FU-138/FU-74 row 02 + keeper tables, OL-63 row-05 claim, FU-148/FU-152 pan producers, FU-146 tracked-side). Serialized one-implementer flow; evidence floor; M1 immovable; M2 re-pins only with reason + frame diff.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan, Ghidra MCP read-only, SDL3 smoke (`make game`, DISPLAY=:1).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU75_*.md`, `FU137…FU152`; predecessors `docs/superpowers/plans/2026-10-09-fifa96-m2-phase8-live-loop.md`.

## Global Constraints

- Baseline: `make check` **108/108**; wired 19/80; HEAD `bbb2221` (+plan commit); M1 `09b726b7…`; M2 `2e709151…`.
- Evidence floor: first-hand bytes/addresses; fresh xrefs for census claims; `/FIFA96.EXE`; +0x100000 rule; CALL-byte/word-pair/gate traps.
- Determinism: null backend source of truth; **M1 never moves**; M2 re-pins only for intended upgrades (movement, live pan/goal) with written reason + frame diff.
- No fakes; unreachable → numbered legs; single situation entry; ISO-gated tests skip without the ISO.
- Commits `feat/fix/docs/test`; controller `make check` + tape `cmp` + tree clean before push.

---

### Task 1: T1 — on-screen movement (FU-75 L4.1/L4.2 + L4.6)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c`, `src/fifa96_engine/fifa96_match_handlers.c` (apply `out.forced`/`out.chase` per FU-75 §1.5/§1.6; row-02 `locomotion_restart_target` per FU-138 OL-18), `src/fifa96_loader/fifa96_outfield.c` / `fifa96_keeper*.c` (keeper tables per FU-74 §2 as needed), `tests/test_engine_match_frame.c`, `tests/test_outfield.c`, `docs/ghidra/FU75_*.md` (L4.1/L4.2 status), `docs/ghidra/FU137_dispatch_mechanics.md` (row 02), `docs/ENGINE.md`
- Read: FU-75 L4.1–L4.6, FU-138 (row 02), FU-74 §2, the T3 f-up5/f-up8 input seam (held key → `input_state`), the live probe finding (slot record code 02, vel 0)

**Interfaces:**
- Produces: the live controlled record moves under a held key on the real frame path (forced-decision/chase applied; row 02 target selection); smoke shows on-screen movement (scene-band AE > 0 while holding).
- Consumes: input hold policy (f-up8 T3), mover/track (S1), phase machine.

- [ ] **Step 1:** Failing tests (held key → position delta on the live frame path; row 02 target selection; discriminating on BASE where `out.forced/chase` are computed-not-applied). 2. Run to fail. 3. Implement + errata. 4. Run to pass. 5. Gate `make check` + golden decision (re-pin iff the tape moves — likely not: no held input on the tape; evidence either way). 6. Commit `feat(engine): live movement — forced-decision/chase and row 02 (FU-75 L4.1/L4.2/L4.6)`.

**Gate T1:** headless held-key movement + smoke on-screen movement observed.

---

### Task 2: T2 — live carrier producers (kick target)

**Files:**
- Modify: `src/fifa96_engine/fifa96_handlers.c` / `fifa96_match_entities.c` / `fifa96_match_run.c` (row-05 carrier claim per OL-63; ball-pair possession producers; the ball spawn/possession chain), `tests/test_engine_match_handlers.c`, `tests/test_engine_match_frame.c`, `docs/ghidra/FU142_installer_arms_scope.md` (OL-63 status), `docs/ENGINE.md`
- Read: OL-63 (row-05 claim `0x7F1FF`), FU-147 (possession flips, code-5 claim), S1/T3 f-up8 (kick path needing a carrier)

**Interfaces:**
- Produces: a possessable ball in live play — the slot/carrier record reaches code 5 so the wired kick (row 07) fires on the real path; smoke kick has a live carrier (still scene-visible only if rendering shows the ball, else headless evidence).
- Consumes: T1 movement, possession/slot seam, T3 f-up8 kick.

- [ ] **Step 1:** Failing tests (live fixture: possession → code 5 → kick release → row 07 → ball impulse). 2. Run to fail. 3. Implement + errata. 4. Run to pass. 5. Gate `make check` + golden decision. 6. Commit `feat(engine): live carrier and possession producers (OL-63)`.

**Gate T2:** kick has a live carrier; driven end-to-end in a headless fixture.

---

### Task 3: T3 — pan origin and auto-camera (live pan → tape goal)

**Files:**
- Modify: `src/fifa96_engine/fifa96_camera.c` / `fifa96_match_run.c` / `fifa96_match_handlers.c` (nine remaining `FUN_00071C94` callers — the gameplay rows that set follow-cam events; `walk_gate` producer per FU-148 §12.2 leg; `FUN_00071DF4` every-frame auto-camera per FU-148/T4; half-line band reach), `tests/test_camera.c`, `tests/test_engine_match_frame.c`, `tests/test_engine_m2.c` (+ golden **re-pin expected**), `docs/ghidra/FU148…` (OL-T11-79 close/narrow), `docs/ghidra/FU152…`, `docs/ENGINE.md`
- Read: FU-148 §2/§5.2/§12, FU-152 §2, FU-145 (armer), T2 f-up8 (pan rate/jitter/arm-A), T4 f-up8 (natural goal + tracked-side)

**Interfaces:**
- Produces: the camera pans/auto-follows from live producers during the match → set-piece arming and, if the tracked-side pick lands, a **natural tape goal** (M2 re-pin with reason + frame diff); tracked-side pick (`[0x1590CC]`/`[0x159901]`) ported if required for the post id.
- Consumes: T2 f-up8 pan step; T4 f-up8 chain; T1 f-up9 movement.

- [ ] **Step 1:** Failing tests (live pan from a real row event; auto-camera; natural goal end-to-end including tracked-side). 2. Run to fail. 3. Implement + errata. 4. Run to pass. 5. Gate `make check` + golden decision (natural tape goal → re-pin with frame diff; else evidence). 6. Commit `feat(engine,fu148): pan origin, auto-camera, tracked-side (OL-T11-79/OL-87)`.

**Gate T3:** camera pans naturally; natural tape goal or deepest step + legs.

---

### Task 4: T4 — acceptance v9 + whole-plan review

**Files:**
- Modify: `tests/test_engine_m2.c` (+ golden iff moved), `docs/ENGINE.md`, `README.md`, `docs/screens/*`
- Read: spec §5; tape v8 provenance

**Interfaces:**
- Produces: tape v9 (movement/carrier/pan assertions; natural goal pinned if landed); each remaining forcing with its leg; smoke with **visible movement + kick** (the f-up8 carry closed); final-review material.

- [ ] **Step 1:** Updated test. 2. Run. 3. Tape/smoke/docs. 4. Run to pass. 5. Gate `make check` + smoke. 6. Commit `test(engine): phase-9 acceptance tape v9 and docs`. Then whole-plan final review; fix wave; close.

**Gate T4:** tape v9 green; smoke movement/kick honest; final review MERGE-READY.

---

## Self-Review Notes

- **Coverage:** movement L4.1/L4.2/L4.6 → T1; carrier/possession → T2; pan/auto-camera/tracked-side → T3; acceptance → T4. Deferred per f-up8 final review: FU-150 fouls/offside legs, FU-152 overlays/replay legs, FU-149/151 residual legs, palette translation content.
- **Dependency order:** T1 → T2 → T3 → T4 (T3's tape goal wants movement/carrier; serialize).
- **Split rule:** any task exceeding a reviewer-sized unit splits at the row/producer boundary and ledgers it.
- **Determinism:** expected re-pins: T3 (live pan/auto-camera and/or natural goal — the first since S1) and possibly T1/T2 if live inputs enter the tape; each with reason + frame diff; M1 never moves.
