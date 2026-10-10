# FIFA 96 M2 phase-10 — visible layer: ball staging, follow-cam, band-reachable tape goal

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Serialized tasks.

**Goal:** Make the live match visually complete and let the natural goal legitimately reach the tape: (1) port the ball-staging producers (`FUN_0007A490`/`FUN_0007A084`) so the high-ball event fires → the follow-cam visibly follows and the ball renders on screen (on-screen kick); (2) close the movement/pan carry-ons (L4.1 unbound-record walk, `0x105278` config-cell producers, the remaining `FUN_00071C94` callers) so live paths widen; (3) build a **band-reachable** acceptance (live-driven goal scenario where lane enters the half-line band and the camera pans) so the natural goal reaches the tape with a legitimate M2 re-pin; (4) acceptance v10 with visible-movement/kick/follow smoke.

**Architecture:** Ports from the named FU legs (FU-148 §13.5 ball-staging leads, FU-75 L4.1, FU-148 config producers/callers, FU-145 armer, FU-146/FU-142 chain). Serialized one-implementer flow; evidence floor; M1 immovable; M2 re-pins only with reason + frame diff; the re-pin is **expected and welcome** in T3 when the tape genuinely pans/scores.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan, Ghidra MCP read-only, SDL3 smoke (`make game`, DISPLAY=:1).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU148…`, `FU152…`, `FU145…`, `FU142…`, `FU75…`; predecessors `docs/superpowers/plans/2026-10-09-fifa96-m2-phase9-live-pad-loop.md`.

## Global Constraints

- Baseline: `make check` **108/108**; wired 21/80; HEAD `a07dc4f` (+plan commit); M1 `09b726b7…`; M2 `2e709151…`.
- Evidence floor: first-hand bytes/addresses; fresh xrefs for census claims; `/FIFA96.EXE`; +0x100000 rule; CALL-byte/word-pair/gate traps.
- Determinism: null backend source of truth; **M1 never moves**; M2 re-pins only for intended upgrades with written reason + frame diff.
- No fakes; unreachable → numbered legs; single situation entry; ISO-gated tests skip without the ISO.
- Commits `feat/fix/docs/test`; controller `make check` + tape `cmp` + tree clean before push.

---

### Task 1: T1 — ball staging producers (visible follow-cam + ball)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c` (ball-staging call path), `src/fifa96_loader/fifa96_ball.c` / relevant loader (the `FUN_0007A490`/`FUN_0007A084` staging bodies per first-hand derivation), `src/fifa96_engine/fifa96_match_entities.c` (ball entity staging/height), `tests/test_engine_match_handlers.c`, `tests/test_engine_match_frame.c`, `docs/ghidra/FU148_presentation_hud_camera.md` (§13.5 status), `docs/ghidra/FU142_installer_arms_scope.md` (OL-88 status), `docs/ENGINE.md`
- Read: FU-148 §13.5 (ball-staging leads), FU-142 OL-88, the f-up8 T3 high-ball trigger (`0x739DC hi>0x10` gate) and f-up9 T3 arm (`0x71E1C..`, `hi>0xF0`), the ball-render path

**Interfaces:**
- Produces: the high-ball staging event live (ball height/time staging) → `FUN_00071DF4` follow arm fires → **visible follow-cam**; the ball renders on screen during play (on-screen kick becomes visible); smoke shows cam follow + ball.
- Consumes: f-up9 pan step/arm, row-04/1E events, the ball pair/possession seam.

- [ ] **Step 1:** Failing tests (staging → high-ball gate → follow arm; ball render pixels; discriminating vs BASE). 2. Run to fail. 3. Implement + errata. 4. Run to pass. 5. Gate `make check` + golden decision + **smoke** (DISPLAY=:1: follow-cam visible? ball visible? record `p10-t1-*` shots). 6. Commit `feat(engine,fu148): ball staging — follow-cam and ball render (OL-88)`.

**Gate T1:** visible follow-cam + on-screen ball during live play (or the deepest provable step with legs).

---

### Task 2: T2 — live-path carry-ons (L4.1, config producers, remaining pan callers)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` / `fifa96_match_handlers.c` (L4.1 unbound-record walk per FU-75 §11), `src/fifa96_loader/fifa96_input.c` (`0x105278` config-cell producer per FU-148 §13), `src/fifa96_engine/fifa96_camera.c` (remaining `FUN_00071C94` callers, walk-gate config), `tests/*`, `docs/ghidra/FU75_*.md`, `docs/ghidra/FU148…`, `docs/ENGINE.md`
- Read: FU-75 L4.1, FU-148 §12/§13 (config cells read-only census; producers), FU-152 §2

**Interfaces:**
- Produces: the unbound-record walk (movement policy for records without a slot), config-cell producers (walk gate/reflect bits) so range words/pan gates can fire from real settings, remaining pan callers wired where reachable.
- Consumes: T1 staging; f-up9 movement.

- [ ] **Step 1:** Failing tests (unbound-record walk; config producers → walk gate; pan caller wiring). 2. Run to fail. 3. Implement + errata. 4. Run to pass. 5. Gate `make check` + golden decision. 6. Commit `feat(engine): live-path carry-ons — unbound walk, config producers (FU-75/FU-148)`.

**Gate T2:** wider live paths (unbound walk + config gates real).

---

### Task 3: T3 — band-reachable acceptance (natural tape goal)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` / scripts as needed to make the acceptance sequence drive lane into the half-line band (live-driven or scripted scenario per spec §5 conventions), `tests/test_engine_m2.c` (+ golden **re-pin expected**), `tests/golden/engine/m2-frames.txt`, `docs/ENGINE.md`
- Read: spec §5 (tape conventions: declared forcing + reasons), f-up8 T4 natural-goal chain, f-up9 T3 no-re-pin evidence (lane ~1460+)

**Interfaces:**
- Produces: an acceptance sequence where the live chain (kickoff → hold movement → pan band → armer → situation 6 → queue → handler → score) runs with **lane in band**, so a natural goal reaches the tape; M2 re-pins with written reason + frame diff (the first since S1); every forcing listed.
- Consumes: T1/T2 live paths, the f-up8/f-up9 chains.

- [ ] **Step 1:** Failing test (the sequence scores naturally; golden diff exists). 2. Run to fail. 3. Implement/script + errata. 4. Run to pass. 5. Gate `make check` + **re-pin** (reason + frame diff summary in ENGINE.md/tape provenance). 6. Commit `test(engine): band-reachable natural goal — tape re-pin`.

**Gate T3:** natural goal on the tape (or the deepest reachable with exact remaining blockers recorded — do not fake lane).

---

### Task 4: T4 — acceptance v10 + whole-plan review

**Files:**
- Modify: `tests/test_engine_m2.c` (+ golden iff moved), `docs/ENGINE.md`, `README.md`, `docs/screens/*`
- Read: tape v9 provenance

**Interfaces:**
- Produces: tape v10 (staging/follow/band-goal assertions); remaining forcings with legs; **smoke with visible movement + kick + follow-cam** (each honestly observed); final-review material.

- [ ] **Step 1:** Updated test. 2. Run. 3. Tape/smoke/docs. 4. Run to pass. 5. Gate `make check` + smoke. 6. Commit `test(engine): phase-10 acceptance tape v10 and docs`. Then whole-plan final review; fix wave; close.

**Gate T4:** tape v10 green; smoke visible-layer honest; final review MERGE-READY.

---

## Self-Review Notes

- **Coverage:** ball staging/follow/render → T1; live-path carry-ons → T2; band-reachable tape goal → T3; acceptance → T4. Deferred: FU-150 fouls/offside legs, FU-152 overlays/replay legs, palette translation content, L4.3–L4.6 residuals, OL-T11-79 remaining callers beyond T2's reach.
- **Dependency order:** T1 → T2 → T3 → T4 (T3's band goal wants T1's follow + T2's gates).
- **Split rule:** any task exceeding a reviewer-sized unit splits at the producer/staging boundary and ledgers it.
- **Determinism:** expected M2 re-pin ONLY in T3 (band goal) — with reason + frame diff; T1/T2 dormant-or-not per evidence; M1 never moves.
