# FIFA 96 M2 phase-11 — visible ball: height producer, long-ball key, code-16 machine

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Serialized tasks.

**Goal:** make the ball *visible* and the follow-cam *live*: (1) port the `0x157750` ball-height producer (L14.7) and the `0x40` long-ball key path (L14.6) so the high-ball chain (staging → `FUN_00071DF4` gate `hi>0x10` → follow arm `hi>0xF0`) fires from live pad input; (2) port the code-16 ball-record machine (L14.8) so the ball's open-play record/position is produced and the ball renders on screen; (3) acceptance v11 with live visible ball + follow-cam honestly asserted/observed, and the deferred docs-minors sweep folded in.

**Architecture:** Ports from FU-148 §14.3 (long-ball key `0x40`), §14.4 (`0x157750` height), T1-report/L14.8 (code-16 machine); the f-up9 camera chain (`fifa96_camera_rate_event`, follow arm) and the T1 receive arm (snapshot, zero-when-not-staged) are the consumers. Serialized one-implementer flow; evidence floor; M1 immovable; M2 re-pins only with reason + frame diff.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan, Ghidra MCP read-only, SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU148…` (§14.3/§14.4), `FU145…`, `FU147…`; predecessor `docs/superpowers/plans/2026-10-09-fifa96-m2-phase10-visible-layer.md` and its final review.

## Global Constraints

- Baseline: `make check` **108/108**; wired 21/80; HEAD `4a96f56` (+plan commit). M1 `09b726b7…`; M2 `5524cf40…` (85 lines).
- Evidence floor: first-hand bytes/addresses; fresh xrefs for census claims; `/FIFA96.EXE`; +0x100000 rule; CALL-byte/word-pair/`dword>>16` alias traps (three camera misreads this project came from it).
- Determinism: null backend source of truth; **M1 never moves**; M2 re-pins only for intended upgrades with written reason + frame diff.
- No fakes; unreachable → numbered legs; single situation entry; ISO-gated tests skip without the ISO.
- **Wired census rule:** if a task changes the wired count, recompute the README title percentage `floor(100 × wired/total)` per AGENTS.md in the same commit.
- Commits `feat/fix/docs/test`; controller `make check` + tape `cmp` + tree clean before push. Individual implementers commit and push per their dispatch.

---

### Task 1: T1 — ball-height producer (`0x157750`) + long-ball key (`0x40`)

**Files:**
- Modify: the ball-pairing/possession producer path (`src/fifa96_loader/fifa96_ball_pairing.c` / engine `fifa96_match_run.c` kick path), `src/fifa96_engine/fifa96_match_handlers.c` (the dormant `match_sp_kick(mr, NULL, 0x40u, 0u)` call site), `src/fifa96_engine/fifa96_camera.c` (tail gate/materialisation), tests + `docs/ghidra/FU148…` (§14.3/§14.4), `docs/ENGINE.md`
- Read: L14.6 (the `0x40` key → traj arm; the code-4/0x40 arm is already ported), L14.7 (the `0x157750` height producer), the camera tail gate `[0x157750] != 0` and the follow arm

**Interfaces:**
- Produces: live ball height (`0x157750`) and the long-ball key path, so a live KICK/long-ball pushes traj > 0xF0 → the follow arm fires → **visible follow-cam**; smoke `p11-t1-*` shows the camera following.
- Consumes: T1-phase-10 receive arm, f-up9 pan/arm chain, pad input.

- [ ] **Step 1:** Failing tests (height producer → tail gate live; long-ball key → traj > 0xF0 → follow arm). 2. Run to fail. 3. Derive + implement + errata. 4. Run to pass. 5. Gate `make check` + golden decision + **smoke** (DISPLAY=:1: follow-cam following? record shots). 6. Commit `feat(engine,fu148): ball height + long-ball key — live follow-cam (L14.6/L14.7)`.

**Gate T1:** the follow-cam visibly follows live play (or the deepest provable step with legs).

---

### Task 2: T2 — code-16 ball-record machine (`L14.8`)

**Files:**
- Modify: the ball record/position producer (`src/fifa96_engine/fifa96_match_entities.c` / `fifa96_match_run.c` per first-hand derivation), the render path if needed (`fifa96_match_handlers` render seam), tests, `docs/ghidra/FU148…`, `docs/ENGINE.md`
- Read: L14.8 (code-16 machine: the ball record's open-play position producer), the ball-slot render test from phase-10 T1, the receiving/render path

**Interfaces:**
- Produces: the ball's open-play position live → the ball renders on screen during play (and during/after a kick); smoke `p11-t2-*` shows the ball.
- Consumes: T1's height/key, the T1-phase-10 staging snapshot, the render seam.

- [ ] **Step 1:** Failing tests (ball position produced in live play; ball pixels drawn). 2. Run to fail. 3. Derive + implement + errata. 4. Run to pass. 5. Gate `make check` + golden decision + smoke. 6. Commit `feat(engine,fu148): code-16 ball machine — on-screen ball (L14.8)`.

**Gate T2:** the ball is visibly drawn in live play (or the deepest provable step with legs).

---

### Task 3: T3 — acceptance v11 + deferred docs-minors sweep

**Files:**
- Modify: `tests/test_engine_m2.c` (+ golden iff it genuinely moves), `tests/golden/engine/m2-frames.txt`, `docs/ENGINE.md`, `README.md`, `docs/screens/*`, and the seven deferred comment/doc minors: `tests/test_engine_m2.c:1-9` (header v9→v11), `include/fifa96_loader/fifa96_input.h:33-36`, `include/fifa96_engine/fifa96_match_run.h:291-295`, `include/fifa96_engine/fifa96_match_entities.h:71`, `docs/ghidra/FU148_presentation_hud_camera.md:1152` (+0x8D inversion), `:1178` (timer81 wording), dead exit-path leftovers in the tape test
- Read: the final review of phase-10 (triage list), tape v10 provenance

**Interfaces:**
- Produces: acceptance v11 (live follow-cam/ball strata where live; honest legs where not); the minors sweep; README title percentage reconciled per AGENTS.md if the census moved.
- Consumes: T1/T2 producers.

- [ ] **Step 1:** Updated tests. 2. Run. 3. Sweep + docs + tape decision. 4. Run to pass. 5. Gate `make check` + smoke. 6. Commit `test(engine): phase-11 acceptance tape v11, docs sweep`. Then whole-plan final review; fix wave; close.

**Gate T3:** tape v11 green; smoke honest (ball/follow-cam visible where reached); minors sweep complete; final review MERGE-READY.

---

## Self-Review Notes

- **Coverage:** height/key → T1; ball machine/render → T2; acceptance + sweep → T3. Deferred: remaining six `FUN_00071C94` pan callers, FU-146 leg-8 restart legs (post-goal freeze), stride-nibble `+0x79`, FU-149..152 carried legs, palette translation content.
- **Dependency order:** T1 → T2 → T3 (T2's render wants T1's live height for the high-ball frames).
- **Split rule:** any task exceeding a reviewer-sized unit splits at the producer seam and ledgers it.
- **Determinism:** live producers may move live frames; M2 re-pin only if T1/T2 genuinely change the tape with reason + frame diff; M1 never moves.
