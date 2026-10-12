# FIFA 96 M2 phase-13 — live camera, ball art

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Wave R runs as three parallel READ-ONLY recon subagents; implementation tasks stay serialized (one implementer at a time).

**Goal:** make the camera actually **move live** (the R2-L1 live view-object handler `FUN_0004E834`, the R2-L4 live pan reachability, the T1-L3 bootstrap that raises the height/place off the stand-in) and make the on-screen ball render as a **ball** (T2-L1's art pipeline — today it is a playart player pose), plus the jump/ack residuals (T1-L1 arm writes/live row selector/`0x6EE92` snap; T1-L2 row-07 stage-2 `+0x44` ack) for a repeatable live sequence. Acceptance v13 carries the two cheap riders from phase-12's final review (aux-0x3A regression pin; ISO-path override for the no-ISO tape path).

**Architecture:** Derivations from FU-154 (§5.1 camera statics, §9 legs), FU-153 (§10-11 jump producer + residuals), FU-156 (arms), and the phase-12 T2/T4 reports. Consumers: the camera chain (`fifa96_camera_rate_event`, `FUN_000736AC` pan integrator, `FUN_000700F4` resets, `fifa96_match_run_reset_render`), the scene slot resolver/render seam, the jump arm (`match_run_record_jump_arm`/`arc`). Serialized implementers; parallel read-only recon is the only parallel surface.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan (engine + tests, since phase-12 T3), Ghidra MCP read-only, SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU153…FU156`; predecessor `docs/superpowers/plans/2026-10-11-fifa96-m2-phase12-live-camera-visible-ball.md` + its final review.

## Global Constraints

- Baseline: `make check` **108/108** (sanitized, since phase-12 T3); wired 21/80; HEAD `8f09dff` (+plan commit). M1 `09b726b7…`; M2 `2759dace…` (85 lines, the v12 re-pin).
- Evidence floor: first-hand bytes/addresses; **fresh `get_xrefs_to` for census claims**; `/FIFA96.EXE` (case-sensitive); +0x100000 rule; **alias traps** (`dword[addr]>>16` = word at `addr+2`; `>>24` = byte at `addr+3`) — **five strikes across the project**; verify CALL bytes.
- Determinism: null backend source of truth; **M1 never moves**; M2 re-pins only for intended upgrades with written reason + frame diff (T2's ball art may genuinely change pixels — that re-pin is welcome; T1/T3 live-only paths expect no re-pin).
- No fakes; unreachable → numbered legs; single situation entry; ISO-gated tests skip without the ISO.
- **Wired census rule:** if a task changes the wired count, recompute the README title percentage `floor(100 × wired/total)` per AGENTS.md in the same commit. Status narrative → `docs/PROGRESS.md`.
- Commits `feat/fix/docs/test`; implementers commit and push per dispatch; controller verifies (`make check` + tape `cmp` + tree clean).

---

## Wave R — parallel read-only recon (gate: three FU slices + reports, committed)

R5–R7 run **concurrently**. Each is read-only on the repo: it writes ONLY its new `docs/ghidra/FU15N_*.md` slice and its workspace report; **no git write commands, no edits to existing files, no code**. The controller commits FU-157…FU-159 in one `docs(fu157-fu159)` commit after the wave returns.

### R5 → FU-157: the live camera producer set (unblocks T1)
- Question: what is the **minimum honest producer set** that makes the camera move live end-to-end in open play? Derive each piece:
  - **R2-L1** — the live view-object handler `FUN_0004E834` (+ its behavior blocks): when it runs, what cells it writes, what it changes vs the `FUN_000700F4` reset triples and the stand-in.
  - **R2-L4** — live pan reachability: the `FUN_000736AC` pan integrator's live inputs (row-04 half-line reachability from FU-154 §9/§5.1), and what actually feeds it in open play.
  - **T1-L3** — the bootstrap: what raises `0x157750` (and the place) off zero/the stand-in from live play (the KICK path, the long-ball key, the height producer interplay).
- Starting windows: FU-154 §5.1/§9, FU-96 leg 5 (`FUN_00079F3C` place), `FUN_000709B4`/`fifa96_camera_rate_event`, `FUN_000736AC`, the 29 `FUN_000700F4` reset call sites, the ported `FUN_00070B94` height producer, `fifa96_match_run_reset_render`.
- Output: FU-157 with the ranked minimum set + a concrete T1 wiring recommendation, or the next-hop producer as a numbered leg.

### R6 → FU-158: the ball-slot art pipeline (unblocks T2)
- Question: why does slot 22's resolved art come out as a **playart player pose**, and what natively selects **real ball sprites**? Derive the render path resolving the ball slot (trace: `slot22 anim=3 bank=3 sprite=yes`; the synthetic per-row frame tables named in T2-L1), the appearance/anim row tables (bank/strip/frame selection), and where the ball's bank/strip ID comes from natively (the ball machine's writes, the strip table, the frame-record bank field).
- Starting: FU-148 §16 (the machine), the phase-12 T1/T2 reports, the render/scene slot resolver path (`fifa96_scene_slot_project`/`fifa96_render`, the bank 3 mapping), FU-154 §9 caveat.
- Output: FU-158 + a concrete T2 wiring recommendation (which tables/cells flip the pose to a ball), or the next-hop as a numbered leg.

### R7 → FU-159: the jump/ack residuals (unblocks T3)
- Question: what do T1-L1 and T1-L2 need to make the live sequence repeatable end-to-end?
  - **T1-L1** — the jump arm's remaining writes (velocity/sine/move), the **live row selector** reaching rows `0x5E`/`0x12`/`0x14`/`0x5C` (FU-153 §10-11), and the `0x6EE90/0x6EE92` snap inclusion.
  - **T1-L2** — the row-07 stage-2 `+0x44` ack producer (what clears/sets `byte44` natively, enabling a second kick per possession).
- Starting windows: FU-153 §10-11, `fifa96_match_run.c` jump arm/arc (`match_run_record_jump_arm`/`arc`), `fifa96_action_handlers.c` `kick_machine_07` (byte44 gate), `FUN_0006E8E8`/`FUN_0006E490` frames, the RNG/row-select paths.
- Output: FU-159 with the T3 wiring recommendation for each piece, or numbered legs.

---

### Task 1: T1 — live camera wiring (from R5)

**Files:** per R5's recommendation (engine camera/run paths, possibly `fifa96_camera.c`, `fifa96_match_run_reset_render`, the place/reset call sites); tests + FU-157 status + `docs/ENGINE.md`.
**Produces:** the camera place/pan live in open play → **smoke shows the camera visibly moving/following**.
**Gate T1:** visible live camera (or the deepest provable step with the next-hop as a numbered leg).
- [ ] RED test → implement → GREEN → gate (`make check` + golden decision + smoke `p13-t1-*`) → commit `feat(engine,fu157): live camera — place/pan producers`.

### Task 2: T2 — ball art (from R6)

**Files:** per R6's recommendation (render/scene slot resolver, the ball machine's bank/anim writes, the per-row frame tables); tests + FU-158 status + `docs/ENGINE.md`.
**Produces:** slot 22 resolves to a ball sprite → **smoke shows a ball, not a pose**.
**Gate T2:** ball art visible (or deepest provable + numbered leg).
- [ ] RED test → implement → GREEN → gate + smoke `p13-t2-*` → M2 re-pin with reason + frame diff if pixels genuinely change → commit `feat(engine,fu159): ball-slot art — real ball sprites (T2-L1)`.

### Task 3: T3 — jump/ack residuals (from R7)

**Files:** per R7's recommendation (`fifa96_match_run.c` jump arm/arc, `fifa96_action_handlers.c` byte44, row-select paths); tests + FU-159 status + `docs/ENGINE.md`.
**Produces:** the live sequence (long-ball → airborne receive → descending kick → band-2 follow) repeatable across possessions.
**Gate T3:** the sequence works with repeated kicks (or deepest provable + numbered leg).
- [ ] RED test → implement → GREEN → gate → commit `feat(engine,fu159): jump/ack residuals — repeatable live sequence`.

### Task 4: T4 — acceptance v13 + riders + smoke + docs + final review

**Files:** `tests/test_engine_m2.c` (+ golden iff genuinely moved), `tests/test_engine_match_frame.c` (the aux-0x3A regression pin), `CMakeLists.txt` (the ISO-path override one-liner), `docs/ENGINE.md`, `docs/PROGRESS.md`, `docs/screens/*`.
**Produces:** v13 strata; the two cheap riders (aux-`0x3A` pin: `ramp(0x18)=18`, RED without the constant; ISO-path environment override so the no-ISO tape path is CI-covered); the honest smoke record; remaining forcings with legs.
- [ ] Tests + riders → tape decision → smoke → docs → commit `test(engine): phase-13 acceptance tape v13 and docs` → whole-plan final review (one fix wave max) → close.

---

## Self-Review Notes

- **Coverage:** live camera → R5/T1; ball art → R6/T2; jump/ack residuals → R7/T3; acceptance + riders → T4. Deferred: FU-149..152 carried legs, FU-146 restart legs, palette translation content, the six unreachable pan callers (FU-156), L155.1, non-engine-lib sanitizer topology.
- **Dependency order:** Wave R (parallel) → T1 → T2 → T3 → T4. T2's ball art is independent of T1; T3's repeated-kick work wants T1/T2's state for the smoke.
- **Split rule:** any recon question that cannot bound its answer in one slice → land the bounded part and number the rest as legs (the wave does not stretch).
- **Determinism:** T2's art change likely re-pins M2 with reason + frame diff (the visible-quality upgrade); T1/T3 live-only paths expect no re-pin; M1 never moves.

---

## Errata (execution record, phase-13; added 2026-10-12 after the two-axis review)

- **Wave R grew by one authorized second wave, not a stretched slice.** The
  Split rule above bounds a reconnaissance *slice*; separately, the user
  directed "use 8 agents to speed up", so the controller authorized a second
  read-only recon wave R8–R14 (FU-160…FU-166, commit `ddee100`) ahead of T3 —
  same read-only contract (slice + workspace report only, no repo writes).
  The T1/T3 gate **fallback clauses** are the ones that were taken (visible
  sustained pan and the full live repeatable sequence remain the numbered
  legs R5-L2/FU160-L4 and FU159-L3/L5/L6 — not gate breaches).
- **T3 also landed R14's session seed** (`fifa96_match_run_begin` seeds
  `[0x14C32A] = 0`, the settled ordinary match, FU-166 §5) per the execution
  ruling carrying R8/R9/R10/R14 refinements into T3; the M2 tape keeps
  byte-1 *staged* at the kickoff directive (FU166-L5, keep-staged) so v13
  stands byte-exact.
- **T4 rider #2** landed as this plan's own Files line predicted (the
  `CMakeLists.txt` ISO-path one-liner): `test_engine_m2_noiso` runs from the
  build dir (`WORKING_DIRECTORY ${CMAKE_BINARY_DIR}`), overriding the path
  environment so the no-ISO tape path is CI-covered on the committed tape
  (109/109).
