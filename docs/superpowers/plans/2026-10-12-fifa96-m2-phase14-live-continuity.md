# FIFA 96 M2 phase-14 — live continuity

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Wave R runs as **eight parallel READ-ONLY recon subagents** (user-directed, "use 8 agents to speed up the reverse engineer"); implementation tasks stay serialized (one implementer at a time).

**Goal:** make the live match **continuous** — (a) the sustained follow actually happens in open play (row-0x0E's stage-2 else arm, the ranked pan arm, the live follow-rate coupling, the row-0x5E installer) and (b) the session **survives a goal** (the FU-165 restart chain + the screen/hold periphery + the row-table continuity arms), so the M2 tape no longer ends as a 91-frame vignette. The M2 tape extension is this phase's one candidate re-pin (additive: first 85 lines byte-identical).

**Architecture:** Derivations from FU-160 (§2 stage-2 else arm = FU160-L4; rec #2 row-0x5E), FU-164 (§3 ranked pan arms), FU-165 (§3 leg-0 restart chain + the 9 legs), FU-159 (§4.1/§5.2 + L5/L6), FU-166 (§6 act-2 re-entry), FU-154/157 (camera statics/follow), FU-153/161 (jump arm/impulsion). Consumers: `fifa96_match_action_0E` (else arm), the camera chain (`fifa96_camera_rate_event`, `FUN_000736AC`, `fifa96_match_run_reset_render`), the scene-stage successor/row tables, the goal/hold/restart machinery (`FUN_000935A0`, `FUN_000948AC`, `FUN_00092D8C`, the goal queue 0x8A9E8). Serialized implementers; parallel read-only recon is the only parallel surface.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan (engine + tests), Ghidra MCP read-only (`/FIFA96.EXE`; the authoritative recon tool — rea MCP only as a secondary read-only view, context7 only if SDL3/CMake docs are needed), SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU153…FU174`; predecessor `docs/superpowers/plans/2026-10-11-fifa96-m2-phase13-live-camera-ball-art.md` + its final review + the 2026-10-12 two-axis review fixes.

## Global Constraints

- Baseline: `make check` **109/109** (sanitized); wired 22/80; README title **27%**; HEAD `ad4cd28` (+plan commit). M1 `09b726b7…`; M2 `5f7e7692…` (85 lines, v13).
- Evidence floor: first-hand bytes/addresses; **fresh `get_xrefs_to` for census claims**; `/FIFA96.EXE` (case-sensitive); +0x100000 rule; **alias traps** (`dword[addr]>>16` = word at `addr+2`; `>>24` = byte at `addr+3`) — **five strikes across the project**; verify CALL bytes.
- Determinism: null backend source of truth; **M1 never moves**; **T3's tape extension is the one intended re-pin** (v14; additive — first 85 lines byte-identical; written reason + frame diff). T1/T2 live-only paths expect no re-pin.
- No fakes; unreachable → numbered legs; single situation entry; ISO-gated tests skip without the ISO.
- **Wired census rule:** if a task changes the wired count, recompute the README title percentage `floor(100 × wired/total)` per AGENTS.md in the same commit. Status narrative → `docs/PROGRESS.md`.
- Commits `feat/fix/docs/test`; implementers commit and push per dispatch; controller verifies (`make check` + tape `cmp` + tree clean).

---

## Wave R — parallel read-only recon, eight agents (gate: eight FU slices + reports, committed)

R15–R22 run **concurrently** (the user-directed speed-up surface). Each is read-only on the repo: it writes ONLY its new `docs/ghidra/FU16N_*.md` slice and its workspace report; **no git write commands, no edits to existing files, no code**. The controller commits FU-167…FU-174 in one `docs(fu167-fu174)` commit after the wave returns.

### R15 → FU-167: row-0x0E stage-2 else arm (FU160-L4) (unblocks T1)
- Question: derive `0x82820..0x82A9C` byte-level — the camera-follow/slot-dir arm that runs when the record is NOT camera-tracked (or `word[+0x85] == 0`): entry conditions, every cell written (slot id/dir, camera follow, record fields), its gates, and the stage-3 tail interplay. Rank what a live port needs first.
- Starting windows: FU-160 §2 (the gate bytes `0x82806/0x82812`), FU-156 (the arm census), the ported `fifa96_match_action_0E`, the slot-dir machinery (`match_run_scene_stage`, `[0x157750]`/`[0x1577CA]`), FU-153 §10-11.
- Output: FU-167 + a concrete T1 wiring recommendation, or the next-hop as a numbered leg.

### R16 → FU-168: pan-arm completion (FU-164 ranking) (unblocks T2)
- Question: take FU-164's ranked arms and fully derive the **top-ranked** one (`0x82A6C` family): its installer, its required inputs, every cell it writes, and the minimal wiring that makes the pan reachable live in open play. Next-ranked arms stay as a ranked fallback list.
- Starting windows: FU-164 §2 (fresh census + corrections) and §3 (per-arm requirement lists), FU-156 reachability, the `FUN_00071C94` call sites, `FUN_000736AC`.
- Output: FU-168 + wiring rec, or the next-hop as a numbered leg.

### R17 → FU-169: row-0x5E live installer (FU160 rec #2) (unblocks T1)
- Question: who installs row `0x5E` in live play, and when? Derive the producer chain from the kick/receive paths into `fifa96_match_run_record_anim_install`/successor, and the exact engine wiring rec.
- Starting windows: FU-160 rec #2, FU-159 §5.2 (row tables; `0x5E {5,0,0x0F}`), the row-0x12 f2/f3 sub-arms, `match_run_record_anim_successor`.
- Output: FU-169 + wiring rec, or numbered legs.

### R18 → FU-170: post-goal restart chain + act-2 re-entry (unblocks T3)
- Question: the exact engine-side sequence for the tape's leg-0 restart chain (FU-165 §3): sit-0xC → act-9 → sit-3 + the `0x93DCA` `[0x15B688] == 0xB4` gate (~91 frames). Which FU165 legs block a *live* port (L1 act-9 stage gates, L2 `word[0x15881A]`, L3 the natural leg, L7 measurement)? Fold FU166-L6's act-2 re-entry semantics (does the restart re-enter the armed split?).
- Starting windows: FU-165 §1–§4 + L1..L9, FU-166 §6, the M2 tape's hold tail, the engine's act/sit machinery (`fifa96_match_run.c`).
- Output: FU-170 with the minimal live sequence + numbered legs, or the next-hop as a leg.

### R19 → FU-171: sustained-follow event inputs (unblocks T2)
- Question: what drives the follow *rate* live? Derive `fifa96_camera_rate_event`'s live inputs, row-04 half-line reachability (FU-154 §9/§5.1) and the `0x157750` place/target events in open play — the coupling that turns the asserted pan into sustained on-screen following. This is the R2-L4/T1-L3 residue.
- Starting windows: FU-154 §5.1/§9, FU-157 (the producer set), the ported `fifa96_camera.c` chain, the rate-event callers, `fifa96_match_run_reset_render`.
- Output: FU-171 + coupling rec, or the next-hop as a numbered leg.

### R20 → FU-172: row-table continuity arms (unblocks T3)
- Question: what do FU159-L5/L6 (the terminal-row movement/steering tables + the `+0x46` lean flags-bit-2 nuance) and the row-0x12 f3/f5 sub-arms actually require, and which of them the jump/receive/restart chain needs for full continuity? Rank and derive the top pieces.
- Starting windows: FU-159 §4.1/§5.2 + legs L5/L6, FU-153 §10-11, FU-161 §5.4, the row tables (`0x12 {last 5, flags 0, next 0x0F}`, `0x14`, `0x5C`, `0x5E`), `match_run_record_anim_successor`.
- Output: FU-172 + ranked wiring rec, or numbered legs.

### R21 → FU-173: post-goal screen/hold periphery (bounds T3)
- Question: what must the engine model for the post-goal hold to run and release *naturally* (not just the tape's forced chain): the goal queue ids 5/6 (`0x8A9E8`), the scheduler (`FUN_000948AC`), the installer (`FUN_00092D8C` ← screen `FUN_00038630` case 0xF), the advance (`FUN_000935A0`), and what the port already has. Bounded: if the scheduler explodes, land the bounded part and number the rest.
- Starting windows: FU-165 §1/§2, FU-142 App. L (the scene/score legs), the phase-8/9 goal-chain work, the engine's goal/sit machinery.
- Output: FU-173 + the minimal rec + legs.

### R22 → FU-174: frontier audit (feeds phase-15 + T4 census)
- Question: once continuity lands, which exercised-but-unwired paths break or degrade the live loop (row sub-stages, keeper, ball-machine branches, remaining pan callers), and what is the cheapest next wiring? Also audit the census: which of the 80 rows would move the wired count next, and re-verify the count machinery (fresh `get_xrefs_to`).
- Starting windows: the current ENGINE census line, the six unreachable `FUN_00071C94` arms (FU-156), the row-0x12/0x14 sub-arms, the keeper legs, the ball machine (FU-148 §16).
- Output: FU-174 with the ranked frontier + census audit.

---

### Task 1: T1 — row-0x0E completion + row-0x5E installer (from R15/R17)

**Files:** per the recs (`fifa96_match_handlers.c` else arm, `fifa96_match_run.c` installer/successor paths); tests + FU-167/FU-169 status + `docs/ENGINE.md`.
**Produces:** the jump selector's stage-2 else arm and the live row-0x5E install run end-to-end in the live chain.
**Gate T1:** else arm + row-0x5E install covered by RED/GREEN live tests (or the deepest provable step + numbered legs).
- [ ] RED test → implement → GREEN → gate (`make check` + golden decision + smoke `p14-t1-*`) → commit `feat(engine,fu167): row-0x0E stage-2 else arm + row-0x5E installer`.

### Task 2: T2 — sustained follow (from R16/R19)

**Files:** per the recs (camera chain + pan arm + coupling); tests + FU-168/FU-171 status + `docs/ENGINE.md`.
**Produces:** sustained on-screen following in open play (the thing phase-13 could only assert in the ISO test).
**Gate T2:** visible sustained pan/follow in the smoke (or deepest provable + numbered leg).
- [ ] RED test → implement → GREEN → gate + smoke `p14-t2-*` → commit `feat(engine,fu168): sustained follow — pan arm + rate coupling`.

### Task 3: T3 — post-goal continuation (from R18/R20/R21)

**Files:** per the recs (goal/hold/restart chain, row-table arms, tape extension); tests + FU-170/FU-172/FU-173 status + `docs/ENGINE.md`.
**Produces:** the match continues past a goal live (the hold releases, play restarts); the M2 tape extends additively (v14) iff the frames genuinely move.
**Gate T3:** the tape/live session continues post-goal (or the deepest provable step + numbered legs); M2 v14 additive re-pin with written reason + frame diff; M1 never moves.
- [ ] RED test → implement → GREEN → gate + smoke `p14-t3-*` → tape decision → commit `feat(engine,fu170): post-goal continuation — restart chain`.

### Task 4: T4 — acceptance v14 + riders + smoke + docs + final review

**Files:** `tests/test_engine_m2.c` (+ golden iff genuinely moved), `docs/ENGINE.md`, `docs/PROGRESS.md`, `docs/screens/*`.
**Produces:** the v14 strata (or v13-held with recorded reason); the cheap riders the T1–T3 reviews surface (regression pins for the else arm / 0x5E install / restart chain); the honest smoke record; remaining forcings with legs; census recomputed if wired moved.
- [ ] Tests + riders → tape decision → smoke → docs → commit `test(engine): phase-14 acceptance tape v14 and docs` → whole-plan final review (one fix wave max) → close.

---

## Self-Review Notes

- **Coverage:** row-0x0E completion → R15/R17/T1; sustained follow → R16/R19/T2; post-goal continuation → R18/R20/R21/T3; acceptance + riders → T4; the frontier/census audit → R22 feeds phase-15 and T4's census check.
- **Dependency order:** Wave R (parallel, eight) → T1 → T2 → T3 → T4. T2's pan work is independent of T1; T3 wants T1/T2's live state for the restart smoke.
- **Split rule:** any recon question that cannot bound its answer in one slice → land the bounded part and number the rest as legs (the wave does not stretch). R21 is the designated explosion risk.
- **Determinism:** T3's additive tape extension is the only intended re-pin (v14: first 85 lines byte-identical, reason + frame diff); T1/T2 expect none; M1 never moves.
- **Tooling:** ghidra-mcp is the sole evidence source (AGENTS.md rules); rea MCP only as a secondary read-only view; context7 only if SDL3/CMake docs become relevant.
