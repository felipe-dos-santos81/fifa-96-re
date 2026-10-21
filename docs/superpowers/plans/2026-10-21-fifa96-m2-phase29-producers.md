# FIFA 96 M2 phase-29 Stage-2 — the visible follow camera (the live Y + the event chain)

> **For agentic workers:** REQUIRED SUB-SKILL: `superpowers:subagent-driven-development`. Steps use checkbox (`- [ ]`).

**Goal:** Make the follow/pan camera **visibly move in `make game`**: land the keeper-band camera-Y delivery (FU307), the chaser corrections + the one-shot re-arm (FU306), the four `[0x157750]` stand-ins and the keeper `follow_speed` seam (FU308), and the event chain + the relative-AE acceptance (FU309) — with the natural-play reach proved or the Layer-2 waiver honestly pre-authorized.

**Architecture:** the camera-Y cell (native `[0x157750]`) is the crux: the engine already carries the live installer route (ladder CX=4 → row `0x1B` → the re-place band = `pos_y + 0x38`) but splits the cell (`keeper_cam_y` vs `render.camera.pos_y`); the four stand-ins and the seam currently read derived cells. The plan unifies the consumers onto the live cells, lands the row-`0x1E` raw-store cuts, fixes the chaser's ramp parameter, then proves the follow window end-to-end before the visible capture.

**Tech Stack:** C (strict `-Wall -Wextra -Werror`, ASan/UBSan), the repo's frame-tape tests, SDL3 `make game`, Ghidra MCP read-only.
**Spec:** `docs/superpowers/specs/2026-10-21-fifa96-m2-phase29-visible-follow-cam-design.md` (§3, §5, §6, §8).
**Authority:** the phase-29 wave `docs/ghidra/FU306..FU309` (reviewed, errata round 1) + the phase-28 register.

## Global Constraints

- `make check` green (strict; ASan/UBSan; ~143 s → ≥600 s timeout); work on `main`; commit per task; push per task.
- M1 `09b726b7…` immutable; **M2 `b281a9b2…` byte-identical** — the seam (T1) and the cell unification (T3) change live CX/pan inputs; each task runs `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` and **stops on a diff** (a written reason + frame diff before any re-pin).
- The live-chain rule: a fixture that stages a gate is a fake; staging a producer's *input* is legitimate; unraised surfaces are numbered LIVE-LEGs.
- The census holds `60/80 = 75%` (no row joins; the four-doc same-commit rule if one does).
- The keeper band is in scope when installer-proven (§8.1); the R11-L4 remainder, the set-piece/free-kick/corner routes and the throw-in cut stay **Out**.
- Ghidra read-only; no fakes.

## Review Focus (the input classes most likely to bite)

1. **The natural CX=4 reach** (the crux): the fixture GO is not a natural-play GO — T3 must prove the natural reach or record FU307-L1 and pre-authorize the §6 waiver.
2. **The seam's CX=1 flip**: `follow_speed = camera.speed` changes the keeper stage select — the M2 gate + the unit pin.
3. **The single-cell unification**: the arc/keeper-band consumers of `camera.pos_y` vs `keeper_cam_y` — no double-writer.
4. **The window length**: 52 frames at camY `0x70` / 54 at `0x50`, plus the pan-counter lockout — a test that assumes 52 at `0x50` is wrong.
5. **The AE attribution**: the same-commit cell assertions are the witness; the pan pair is corroboration (baseline-green).

---

### Task 1: The keeper `follow_speed` seam

**Files:** `src/fifa96_engine/fifa96_match_run.c` (the frame bind), `fifa96_match_entities.c` (the seam); Tests `tests/test_keeper_cpu.c`, `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED** — the unit pin: `camera.speed = 9` → the CX=1 stage yields 1; `0xA` → 0 (the native `0x78046..0x7804E`).
- [ ] **Step 2: Implement** `f->follow_speed = (int16_t)mr->render.camera.speed;` (keep `camera.follow_speed` for the `0x739CE` gate).
- [ ] **Step 3:** PASS; `make check`; **M2 `cmp` (STOP on a diff)**.
- [ ] **Step 4: Commit** `fix(fu308): the live follow_speed seam`.

### Task 2: The `[0x157750]` stand-ins

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (rows 05/06/08), `fifa96_match_run.c` (the chase gate); Test `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED** — each site reads `(int32_t)mr->render.camera.pos_y` (the native `dword[0x157750]`), not the zero stand-in; the chase height widened to int32.
- [ ] **Step 2: Implement** the four sites (`handlers.c:2813`, `:3086`, `:2977`; `run.c:912`).
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`.
- [ ] **Step 4: Commit** `feat(fu308): the live 0x157750 stand-ins`.

### Task 3: The keeper-band camera-Y delivery + the natural reach

**Files:** the keeper claim module (`src/fifa96_loader/fifa96_keeper_claim.*`), `fifa96_match_handlers.c`, `fifa96_match_entities.c`, `fifa96_camera.*`; Test `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED** — the row-`0x1E` cuts (`0x75A0F` = 0x38, `0x75EEA`, `0x75F86` = 0x80) deliver a `cam_y_written` raw store to `render.camera.pos_y`; the fixture asserts the band lands.
- [ ] **Step 2: Implement** the raw-store output + the single-cell unification (the keeper band → the render camera Y; no double-writer; the arc untouched).
- [ ] **Step 3: The natural reach** — prove CX=4 is reachable from natural play (the tape / `make game` trace) or record FU307-L1 and pre-authorize the §6 Layer-2 waiver.
- [ ] **Step 4:** PASS; `make check`; M2 `cmp` (STOP on a diff).
- [ ] **Step 5: Commit** `feat(fu307): the keeper-band camera-Y delivery (the natural reach proved or waivered)`.

### Task 4: The chaser corrections + the one-shot re-arm

**Files:** `src/fifa96_engine/fifa96_camera.c`, `fifa96_match_run.c`; Test `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED** — `ramp_param = 1` (native `0x70DBA MOV ECX,1`; the engine passes 0); the stale `height_5d` (`run.c:584` vs `:1510`); the 16-bit seed wrap.
- [ ] **Step 2: Implement** the three corrections + the one-shot re-arm test (the `0x7C83E` tracked-skip; the `0x71D27` rebind).
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`.
- [ ] **Step 4: Commit** `fix(fu306): the chaser ramp parameter + the one-shot re-arm`.

### Task 5: The event chain + the Layer-1 gate

**Files:** `tests/test_engine_match_frame.c` (the fixture); `fifa96_match_run.c` if the event staging requires it

- [ ] **Step 1: RED** — the follow/height assertion fails on the phase-29 baseline: a real receive event (traj `0x100`, a nonzero vector, `has_slot` `0x73B65`) + the mid-band Y → the follow arm fires → the camera pos/pan cells move over the window (52 frames at camY `0x70`, 54 at `0x50`).
- [ ] **Step 2: GREEN** with mutation-RED riders on each producer (the seam, the stand-ins, the band, the chaser, the event).
- [ ] **Step 3:** the pan assertion labeled **corroboration** (baseline-green) or made genuinely RED.
- [ ] **Step 4: Commit** `test(fu309): the follow chain — GREEN (Layer 1)`.

### Task 6: The Layer-2 acceptance + the close-out

**Files:** `docs/screens/p29-*`; `docs/ENGINE.md`, `docs/PROGRESS.md`, `AGENTS.md:90` (only if the census moves)

- [ ] **Step 1:** `make game` (`DISPLAY=:1`): the same-clock pan/still pairs (`960x591+0+0`); the relative gate `AE_pan > AE_still` (the p28 fresh datum: 1582 vs 2052); the cell assertions as the attribution (the pan AE as corroboration).
- [ ] **Step 2:** the honest verdict — visible follow, or the §6 waiver (the FU307-L1 natural-reach leg + the remaining legs).
- [ ] **Step 3:** the leg register (append-only): FU306 (the chaser; L1–L8), FU307 (the band; L1–L6), FU308 (the stand-ins + the seam; L1–L5), FU309 (the event source + the acceptance; L1–L8).
- [ ] **Step 4:** `make check`; M1 byte-identical; commit `docs(phase29): the close-out — the leg register`.

---

## Self-Review

- **Spec coverage:** §3.1 = T4; §3.2 = T3; §3.3 = T1/T2; §3.4 = T5/T6; §8.1 = T3; §8.2 = T5; §8.3 = T6; §8.4 = T4.
- **The honest risk:** the natural CX=4 reach — T3 proves it or pre-authorizes the waiver (the wave review's Top-1).
- **Placeholders:** none.

## Execution Handoff

**1. Subagent-Driven (recommended).** **2. Native.**
