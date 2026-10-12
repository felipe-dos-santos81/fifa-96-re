# FIFA 96 M2 phase-15 — census frontier

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Wave R runs as **eight parallel READ-ONLY recon subagents** (user-directed, "use 8 agents to speed up the reverse engineer"); implementation tasks stay serialized (one implementer at a time).

**Goal:** raise the derived completion percentage by honestly wiring the next frontier — the remaining install-live action rows (**0x1F, 03, 09, 0x19**), their second-order installs (**0x1B, 0x0D**), and the native **act/phase dispatch seam** (which unlocks action row **0x17** and the phase-table rows) — each proven by RED/GREEN tests, mutation-RED discriminators and, where reachable, an ISO-gated live chain.

**Correction vs. the planning conversation:** FU-174 (the phase-14 wave R22 snapshot) ranked **0x0B** among the movers, but row 0x0B was wired by phase-14 T1 (FU-169) — the current baseline is **24/80**, not the 22/80 FU-174 recorded. The honest first-order set for phase-15 is therefore **0x1F, 03, 09, 0x19** (0x0B already done).

**Architecture:** Derivations from FU-174 (§2 installers, §4 ranking, §6 census), FU-140 (keeper rows 0x19/0x1A/0x1B/0x1C/0x1F), FU-136/FU-138/FU-139/FU-142 (outfield rows 03/09/0x17 bodies, OL-19/OL-22), FU-165 §6 (the native act machine `FUN_000888FC` + the `0x8B669` clock-tail tick), FU-137 (the two dispatch tables). Consumers: `fifa96_match_handlers.c` bodies + the action/phase tables, a new act-machine seam in `fifa96_match_run.c` (or `fifa96_match_act.{c,h}` if R23 justifies), the helper gaps in `fifa96_outfield.c`/`fifa96_action_handlers.c`. Serialized implementers; parallel read-only recon is the only parallel surface.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan (engine + tests), Ghidra MCP read-only (`/FIFA96.EXE`; the authoritative recon tool — rea MCP only as a secondary read-only view, context7 only if SDL3/CMake docs are needed), SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU175…FU182`; predecessor `docs/superpowers/plans/2026-10-12-fifa96-m2-phase14-live-continuity.md` + its final review; frontier map `docs/ghidra/FU174_frontier_audit.md`.

## Global Constraints

- Baseline: `make check` **109/109** (sanitized); wired **24/80**; README title **30%**; HEAD `1971a59`. M1 `09b726b7…` (691 lines); M2 `7b4b241a…` (v14, 792 lines).
- Census arithmetic (this phase's target): action non-NULL 24 → **31** if all seven action rows land (0x1F, 03, 09, 0x19, 0x1B, 0x0D, 0x17); **plus** any phase-table rows R23/R30 prove live-reachable through the new act seam. Each task recomputes `floor(100 × wired/80)` when its count moves.
- Evidence floor: first-hand bytes/addresses; **fresh `get_xrefs_to` for census claims**; `/FIFA96.EXE` (case-sensitive); +0x100000 rule; **alias traps** (`dword[addr]>>16` = word at `addr+2`; `>>24` = byte at `addr+3`) — **five strikes across the project**; verify CALL bytes.
- Determinism: null backend source of truth; **M1 never moves**; **T1 is the one tape risk** (a new phase-dispatch call site in the frame body) — M2 stays v14 if byte-identical, else additive re-pin v15 with a written reason + frame diff. T2/T3 are expected tape-dormant (install-live, not tape-dispatched) → no re-pin.
- No fakes; unreachable → numbered legs; single situation entry; ISO-gated tests skip without the ISO.
- **Wired census rule:** if a task changes the wired count, recompute the README title percentage in the same commit per AGENTS.md; keep it consistent with the `docs/ENGINE.md` census line. Status narrative → `docs/PROGRESS.md`.
- Commits `feat/fix/docs/test`; implementers commit and push per dispatch; controller verifies (`make check` + tape `cmp` + tree clean).

---

## Wave R — parallel read-only recon, eight agents (gate: eight FU slices + reports, committed)

R23–R30 run **concurrently** (the user-directed speed-up surface). Each is read-only on the repo: it writes ONLY its new `docs/ghidra/FU17N_*.md` slice and its workspace report; **no git write commands, no edits to existing files, no code**. The controller commits FU-175…FU-182 in one `docs(fu175-fu182)` commit after the wave returns.

### R23 → FU-175: the native act/phase dispatch seam (the enabler) (unblocks T1)
- Question: derive `FUN_000888FC` + the `0x8B669..0x8B672` clock-tail tick + `[0x158808]` + how an act invokes a phase handler (`[0x1C]` per-record, `FUN_0008D098`'s walk) — the act-slot lifecycle, the situation→act map, and the exact engine wiring that gives `fifa96_match_dispatch_phase` a live caller. Rank which phase rows become honestly live-reachable once the seam lands.
- Starting windows: FU-165 §6 (the act machine legs), FU-137 §2/§3 (the phase reader `FUN_0006D920` @ `0x6D9B3`), the engine's phase machines (`match_run.c` phase drive / `[0x157A4A]`), FU-174 §7 note 2.
- Output: FU-175 + a concrete T1 wiring recommendation (new file vs. `match_run.c`), the honest phase-row reachability list, or the next hop as a numbered leg.

### R24 → FU-176: row 0x1F keeper dive/arm body (unblocks T2)
- Question: derive `0x76380..0x765C2` byte-level (4 stages) — every cell/gate written, the phase gate, the arm-step/target/input-decide wiring onto the ported `fifa96_keeper_dive_target`/`_arm_step`/`_arm_camera`/`_input_decide` — plus its second-order **0x1B install** at `0x76550`. Rank the minimal live port.
- Starting windows: FU-174 §2 (installer live: phase-6 arm `match_run.c:2445`), FU-140 §2/§3 + OL-36, FU-151 (the keeper family), the phase-6 penalty arm.
- Output: FU-176 + T2 wiring rec, or the body remainder as numbered legs.

### R25 → FU-177: row 03 hold/clamp body (unblocks T3)
- Question: derive the row-03 body remainder (OL-19) — the `[rec+0x1C]` phase-handler call, the `FUN_00079F3C` camera placement and the phase-7 ball scan — onto the ported FU-138 helpers (`locomotion_hold`/`clamp_placement`/`placement_counter`/`phase1_clamp`). Note the `[rec+0x1C]` link to R23's phase seam.
- Starting windows: FU-174 §2 (installer live: the L4.1 walk + row-04 active records), FU-138, OL-19, `match_run.c` (the forced-decision walk).
- Output: FU-177 + T3 wiring rec, or numbered legs.

### R26 → FU-178: row 09 sequence/steer body (bounded) (unblocks T3)
- Question: derive `0x80A00..0x81065` (597 insns) — the stage-0 slice and the `0x809F0` 4-arm table boundary (FU-136 "arms unported") — plus its second-order **0x0D install** at `0x80F61`. **Bounded:** land the stage-0 + the arm table map, rank the arms; do NOT stretch (the designated explosion risk).
- Starting windows: FU-174 §2 (two live installers: the pad release edge + the wired row-06 RNG arm), FU-136 row-09, FU-81 arm table `0x809F0`, OL-9.
- Output: FU-178 + a bounded T3 wiring rec (deepest provable stage-0) with the arms as numbered legs.

### R27 → FU-179: row 0x19 keeper hold body (unblocks T2)
- Question: derive the row-0x19 body remainder — the predictor block `0x74832..0x74ACC` (OL-33) — onto the ported `fifa96_keeper_hold_*` helpers; every cell/gate, the row's install/fallback arms.
- Starting windows: FU-174 §2 (installer live: the row-04 non-active arm), FU-140 §2/§3 + OL-33, `fifa96_action_handlers.c` (keeper_hold_*).
- Output: FU-179 + T2 wiring rec, or the predictor remainder as numbered legs.

### R28 → FU-180: second-order keeper chain — 0x1B + 0x0D (unblocks T2)
- Question: derive the **0x1B** keeper-reposition/rush body (`0x77423` `FUN_00071C94` caller, keeper_reposition_b_finish) and the **0x0D** body (FU-82 4-arm `0x8250C` + FU-138 `velocity_scale`) — the two installs phase-15's new rows arm — plus the keeper-chain upstream they need (`FUN_000782D0` record-0 machine, `FUN_00076130`, `FUN_00077EAC`, the `0x76B28` steering; OL-34/OL-35). Bounded: what is needed for the install to execute honestly vs. the deep chain.
- Starting windows: FU-174 §2 (`0x76550` 0x1B install) + §5 (the 0x77423/0x77DC6 pan callers), FU-140 OL-34/OL-35, FU-151 family.
- Output: FU-180 + ranked wiring rec (0x1B/0x0D bodies now, deep chain as legs).

### R29 → FU-181: row 0x17 + the phase arms + install-execution seam (unblocks T1/T4)
- Question: derive action row **0x17** (the simple phase-0xB gate, `0x84743 CMP EAX,0xB; JZ` else the `0x849A1` reset tail, FU-81 §2) + the phase handlers `0x1A/0x1E/0x1F` that the R23 act seam unlocks, and the shared **install-execution seam** (FU174-L3): how a dispatched row's `out.install`/invoke-now drains through `fifa96_match_entities_install`.
- Starting windows: FU-174 §3 (T3-R2 route) + §2 (0x17) + §8 legs, FU-81 §2, FU-137 §2 (the `0x7D9A4` installer/dispatch).
- Output: FU-181 + the row-0x17 wiring rec + the phase-arm list, or numbered legs.

### R30 → FU-182: census audit + next frontier (feeds T4 + phase-16)
- Question: re-verify the count machinery with fresh in-session evidence (the two tables, `get_xrefs_to 0x7D9A4`), state the exact post-phase-15 % arithmetic (action 31/80 + any live phase rows), list the remaining honest movers and the blocked/no-move set, and rank the phase-16 frontier.
- Starting windows: FU-174 §1/§6/§7, the ENGINE census line, `fifa96_match_handlers.c` tables.
- Output: FU-182 with the verified census + ranked phase-16 frontier.

---

### Task 1: T1 — native act/phase dispatch seam + row 0x17 (from R23/R29)

**Files:** per the recs (`src/fifa96_engine/fifa96_match_run.c` and/or a new `fifa96_match_act.{c,h}`; `include/fifa96_engine/fifa96_match_handlers.h` + `src/fifa96_engine/fifa96_match_handlers.c` row 0x17; the phase table rows R23 proves live); tests + FU-175/FU-181 status + `docs/ENGINE.md` + README title.
**Produces:** the act machine runs in the live frame body; `fifa96_match_dispatch_phase` has a live caller; action row 0x17 dispatches.
**Gate T1:** the act seam + row 0x17 covered by RED/GREEN + mutation-RED tests; the phase-row census claim only if R23 proves a live-reachable phase handler (else numbered leg, keep the table honest). **Tape decision:** replay M2 — byte-identical → no re-pin (record the check), else additive v15 with reason + frame diff. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate (`make check` + tape `cmp` + smoke `p15-t1-*`) → census/README update → commit `feat(engine,fu175): act/phase dispatch seam + row 0x17`.

### Task 2: T2 — keeper cluster: 0x1F + 0x1B + 0x19 (from R24/R27/R28)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (rows 0x1F/0x1B/0x19 bodies), `include/fifa96_engine/fifa96_match_handlers.h` (table slots), helper gaps in `fifa96_action_handlers.c`/`fifa96_outfield.c`; tests + FU-176/FU-179/FU-180 status + `docs/ENGINE.md` + README title.
**Produces:** rows 0x1F, 0x1B and 0x19 execute end-to-end in the live chain; the 0x1F→0x1B install chain runs.
**Gate T2:** each body RED/GREEN + mutation-RED; the 0x1F→0x1B install proven; ISO-gated live chain where the penalty/keeper route is reachable, else unit fixture + tape-dormancy pin. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p15-t2-*` → census/README update → commit `feat(engine,fu176): keeper cluster — rows 0x1F/0x1B/0x19`.

### Task 3: T3 — outfield cluster: 03 + 09 + 0x0D (from R25/R26)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (rows 03/0x0D bodies; row 09 bounded stage-0), `fifa96_outfield.c`/`fifa96_action_handlers.c` helper gaps; tests + FU-177/FU-178 status + `docs/ENGINE.md` + README title.
**Produces:** rows 03 and 0x0D execute end-to-end; row 09 lands its deepest provable stage-0 with the arm table as numbered legs if it cannot bound in one task.
**Gate T3:** each row RED/GREEN + mutation-RED; row 09's bounded landing + leg register. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p15-t3-*` → census/README update → commit `feat(engine,fu177): outfield cluster — rows 03/0x0D + row 09 stage-0`.

### Task 4: T4 — acceptance v15 + riders + smoke + docs + final review

**Files:** `tests/test_engine_m2.c` (+ golden iff genuinely moved), `docs/ENGINE.md`, `docs/PROGRESS.md`, `docs/screens/*`.
**Produces:** the v15 strata (or v14-held with recorded reason); the cheap regression pins the T1–T3 reviews surface; the honest smoke record; the remaining legs with owners; the census recomputed against R30's audit.
- [ ] Tests + riders → tape decision → smoke → docs → commit `test(engine): phase-15 acceptance tape v15 and docs` → whole-plan final review (one fix wave max) → close.

---

## Self-Review Notes

- **Coverage:** act seam + row 0x17 → R23/R29/T1; keeper 0x1F/0x1B/0x19 → R24/R27/R28/T2; outfield 03/09/0x0D → R25/R26/T3; acceptance + census → R30/T4.
- **Dependency order:** Wave R (parallel, eight) → T1 → T2 → T3 → T4. T2/T3 are independent of each other and of T1's action-row work; sequence T1 first because the act seam is the riskiest (it touches the frame body).
- **Split rule:** R26 (row 09, 597 insns) is the designated explosion risk — bounded stage-0 + numbered arms, the wave does not stretch.
- **Determinism:** T1's act seam is the only tape risk (v15 additive iff the frames move); T2/T3 expect no re-pin; M1 never moves.
- **Census:** baseline 24/80 (NOT FU-174's 22/80 — that was the pre-phase-14-T1 snapshot); target 31/80 + any live phase rows; every moving task recomputes the README % + ENGINE line in its own commit.
- **Tooling:** ghidra-mcp is the sole evidence source (AGENTS.md rules); rea MCP only as a secondary read-only view; context7 only if SDL3/CMake docs become relevant.

---

## Landing notes (post-execution)

> **Task 3 "Produces: rows 03 and 0x0D execute end-to-end" (line 90):** not met
> as written. Honest landing: only row **09** was counted in T3 (its bounded
> stage-0); row **0x0D** was ported byte-complete but is **uncounted** — its
> only installer (row-09 arm 1, `0x80F61`) stays unported (FU178-L1), so the
> action-table slot stays NULL (the rows-27/29/2C precedent); row **03** stays
> **UNSUP** (FU177). The census after the wave is **29/80 = 36%** (28 action +
> the first wired phase row 0x1F), not the self-review's 31/80 target. Recorded
> in the SDD ledger (Ruling 7).
