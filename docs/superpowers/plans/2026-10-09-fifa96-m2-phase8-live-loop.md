# FIFA 96 M2 phase-8 — live gameplay loop: taker rows, camera live feed, kick/hold, natural goals

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Serialized tasks from the frozen slices + fresh-first-hand derivations where marked.

**Goal:** Close the loop from pad to score on the live surface: play the taker/corner row machines (L13), feed the camera live (pan origin, OL-T11-79), give input hold/kick semantics (OL-T4-1 + row 0x19), and let the goal chain fire naturally (OL-87/88/89) — ending with a live-observed score if reachable, or the deepest reachable step with full legs.

**Architecture:** Each task starts from its named requirement source (frozen slice, FU leg, or fresh first-hand derivation where flagged). Shared rules: single situation entry, no fakes, numbered legs, M1 immovable, M2 re-pin only with reason + diff. Serialized → one implementer at a time; controller verifies and pushes.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan, Ghidra MCP read-only, SDL3 smoke (`make game`, DISPLAY=:1).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU145…FU152`; predecessors `docs/superpowers/plans/2026-10-09-fifa96-m2-phase7-ports.md`, `2026-10-08-fifa96-m2-phase7-recon-ahead.md`.

## Global Constraints

- Baseline: `make check` **108/108**; wired 15/80; HEAD `cabcac2` (+plan commit); M1 `09b726b7…`; M2 `2e709151…`.
- Evidence floor: first-hand bytes/addresses; fresh xrefs for census claims; `/FIFA96.EXE`; +0x100000 rule; CALL-byte/word-pair/gate traps.
- Determinism: null backend source of truth; **M1 never moves**; M2 re-pins only for intended upgrades with written reason + frame diff.
- No fakes; unreachable → numbered legs; shared 0xB/situation entries never duplicated; ISO-gated tests skip without the ISO.
- Commits `feat/fix/docs/test`; controller runs `make check` + tape `cmp` + tree clean before push.

---

### Task 1: T1 — taker/corner row machines (L13)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c` / `.h` (rows 0x10 throw-in taker, 0x11 corner taker/`0x12` free-kick + `0x13` penalty taker codes per FU-149 §6 L13 + P1's installed codes), `tests/test_engine_match_handlers.c`, `tests/test_engine_match_frame.c`, `docs/ghidra/FU149_set_pieces_restarts.md` (L13 status), `docs/ENGINE.md`
- Read: FU-149 L13 + §6 (installed codes 0x10/0x11/0x12/0x13/0x1D), FU-137 §6.1 row taxonomy, the phase-arm machinery (P1)

**Interfaces:**
- Produces: the taker/corner row bodies (derive first-hand the row machines' real address ranges — the slice didn't freeze them; record them as an FU-149 appendix/erratum with windows) wired so an armed taker row executes its stage machine and resolves (plays the restart; hands back to phase 2 where the native does).
- Consumes: P1's set-piece routing/arms; the possession/slot seam (S1); phase machine.

- [ ] **Step 1:** Derivation (first-hand windows for rows 0x10/0x11 bodies + their stage tables) then failing tests (armed taker executes; resolution).
- [ ] **Step 2:** Run to fail. 3. Implement + FU-149 appendix/errata. 4. Run to pass. 5. Gate `make check` + golden decision. 6. Commit `feat(engine,fu149): taker row machines (L13)`. 

**Gate T1:** taker/corner rows execute; L13 closes or narrows with evidence.

---

### Task 2: T2 — camera live feed / pan origin (OL-T11-79)

**Files:**
- Modify: `src/fifa96_engine/fifa96_camera.c` / `.h`, `src/fifa96_loader/fifa96_camera.c` / `.h` (FU-71 event bodies `709D0/70DE0/71DF4` + atan walk + tails; `FUN_00071C94` bail gate `[0x157A6C]`; rate/timer producers for the pan), `src/fifa96_engine/fifa96_match_run.c` (frame wiring), `tests/test_camera.c`, `tests/test_engine_match_frame.c`, `docs/ghidra/FU148_presentation_hud_camera.md` (OL-T11-79 status), `docs/ghidra/FU96_camera_objects.md`, `docs/ENGINE.md`
- Read: FU-148 §2/§5.2 + OL-T11-79, FU-152 §2 (handler bodies/atan), the S4 pose-feed seam, `FUN_000736AC` pan integrator (rate words `0x1577C0/C2`)

**Interfaces:**
- Produces: live camera motion — the FU-71 event bodies + pan-rate producers reachable from gameplay/row events; the armer's out-of-bounds path fires naturally when the camera pans (ties into S2's chain).
- Consumes: S4 pose feed; FU-145 armer; FU-147 lane/deltas.

- [ ] **Step 1:** Failing tests (camera rate/event → integrator → armer fires from the real path, not a fixture-only setter; event body semantics).
- [ ] **Step 2:** Run to fail. 3. Implement + errata. 4. Run to pass. 5. Gate `make check` + golden decision (tape re-pin likely iff the live camera moves on the tape). 6. Commit `feat(engine,fu148): camera live feed and pan origin (OL-T11-79)`.

**Gate T2:** natural pan path can arm the goal chain.

---

### Task 3: T3 — input hold/kick semantics (OL-T4-1 + row 0x19)

**Files:**
- Modify: `src/fifa96_engine/platform_sdl3.c` (repeat/hold policy: keep auto-repeat or synthesize hold), `src/fifa96_engine/fifa96_match_input.c` / `.h`, `src/fifa96_engine/fifa96_match_handlers.c` / `.h` (kick/pass/shoot rows reachable from the pad — row 0x19 kick execution path per FU-137 §6), `tests/test_engine_match_input.c`, `tests/test_engine_match_frame.c`, `docs/ENGINE.md`
- Read: OL-T4-1 (auto-repeat drop), the pump/possess chain (S1/FU-147), FU-137 §6 rows, the kick gates (`+0x91` action code family)

**Interfaces:**
- Produces: on-screen movement with held keys (or a documented hold policy) + the kick action executing from the pad (ball impulse / action-code path) — the "play the game" surface.
- Consumes: pad→target→mover seam (S1/T1 f-up5); row-04 staging; possession.

- [ ] **Step 1:** Failing tests (hold synthesis; kick press → derived kick action; no fakes). 2. Run to fail. 3. Implement + errata. 4. Run to pass. 5. Gate `make check` + golden decision. 6. Commit `feat(engine): input hold policy and kick action (OL-T4-1)`.

**Gate T3:** pad moves the record and kicks the ball in a headless fixture; smoke shows on-screen movement.

---

### Task 4: T4 — natural goal chain close-out (OL-87/88/89)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c`, `src/fifa96_loader/fifa96_referee.c` (as the chain dictates), `tests/test_engine_match_frame.c`, `tests/test_engine_m2.c` (+ golden iff a natural goal enters the tape), `docs/ghidra/FU142_installer_arms_scope.md` App. L (status), `docs/ENGINE.md`
- Read: FU-142 App. L (11 writer sites + handlers), FU-146 (consumers), T2's natural pan, the S3 chain

**Interfaces:**
- Produces: a natural goal — pan arms (`T2`) → scanner situation 6 → queue → scheduler → handler → `score_event`; or the deepest reachable step with exact blockers.
- Consumes: T2 camera motion, S2/S3 chain, P1/P2 set pieces.

- [ ] **Step 1:** Failing tests (end-to-end natural goal in a headless fixture with T2's camera path). 2. Run to fail. 3. Implement + errata. 4. Run to pass. 5. Gate `make check` + golden decision (natural goal on the tape = re-pin with diff). 6. Commit `feat(engine): natural goal chain close-out (OL-87/88/89)`.

**Gate T4:** score arises from replicated gameplay (or deepest reachable + legs).

---

### Task 5: T5 — acceptance v8 + whole-plan review

**Files:**
- Modify: `tests/test_engine_m2.c` (+ golden iff moved), `docs/ENGINE.md`, `README.md`, `docs/screens/*`
- Read: spec §5; tape v7 provenance

**Interfaces:**
- Produces: tape v8 (taker rows, live camera, hold/kick, natural goal where reachable); every remaining forcing with its leg; fresh smoke (on-screen movement/kick observable); the plan final-review material.

- [ ] **Step 1:** Updated test. 2. Run. 3. Tape/smoke/docs. 4. Run to pass. 5. Gate `make check` + smoke. 6. Commit `test(engine): phase-8 acceptance tape v8 and docs`. Then final whole-plan review; fix wave if needed; close.

**Gate T5:** tape v8 green; smoke honest; final review MERGE-READY.

---

## Self-Review Notes

- **Coverage:** L13 → T1; OL-T11-79 → T2; OL-T4-1 + row 0x19 → T3; OL-87/88/89 → T4; acceptance → T5. Not covered (later cycles): FU-149 L1–L12 residuals, FU-150 legs 1–11 (settings UI/RNG/SFX identities), FU-151 15 legs, FU-152 14 legs, palette translation pool content, replay strings/glyphs.
- **Dependency order:** T2 before T4 (camera motion is the natural goal armer); T1 before T5; T3 independent; serialize.
- **Split rule:** any task exceeding a reviewer-sized unit splits at the producer/row boundary and ledgers it.
- **Determinism:** expected M2 re-pins: T2 (live camera), T4 (natural goal), T5 if moved — each with reason + frame diff; M1 never moves.
