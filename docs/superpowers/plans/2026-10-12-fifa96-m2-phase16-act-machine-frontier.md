# FIFA 96 M2 phase-16 — act machine + census frontier

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Wave R runs as **eight parallel READ-ONLY recon subagents** (user-directed, "use 8 agents to speed up the reverse engineer"); implementation tasks stay serialized (one implementer at a time).

**Goal:** raise the derived completion percentage honestly on two fronts — (a) port the native **act-machine producer** (`FUN_000888FC`'s upstream: the sit/act producer that gives the phase seam a live phase-row cohort, plus the act-4 sit-2..4 formation-order arm that unlocks action row **0x17**, whose body phase-15 already ported but left uncounted) and (b) wire the remaining install-live action bodies FU-182 §7 ranks (**0x14, 0x22, 0x1A, 0x1C, 0x16, 0x24**), the conditional installer hunt (**0x20, 0x25**) and the completion paths for the two ported-but-uncounted rows (**0x0D**, **03**) — each proven by RED/GREEN tests, mutation-RED discriminators and, where reachable, an ISO-gated live chain.

**Baseline (this plan's, verified at HEAD `44f8083`):** wired **29/80 = 36%** (28 action + the first wired phase row `0x1F`); `make check` **110/110** (sanitized, `-Wall -Wextra -Werror`); README title **36%**; M1 `09b726b7…` (691 lines); M2 `7b4b241a…` (v15 strata label, 792 lines, byte-identical). FU-182 §1/§8.1 is the authoritative census (its `24/80` is the *pre-phase-15* snapshot; the current verified count is `29/80`).

**Architecture:** Derivations from FU-182 (§4 first-order movers 0x14/0x22, §5 second-order 0x16/0x24/0x1A/0x1C/0x20/0x25, §6 blocked set, §7 ranked frontier, §9 legs), FU-175 (the act/phase seam already landed by phase-15 T1), FU-165 §6 (the native act machine legs), FU-140 (keeper chain rows 0x1A/0x1B/0x1C), FU-136 (scatter_celebration / sequence_press_event helpers; OL-9), FU-148 §16.2 (row 0x16 body), FU-173 (FU173-L2/L3 goal-record release), FU-177/FU178 (rows 03/0x0D blockers). Consumers: `fifa96_match_handlers.c` bodies + the action/phase tables, the act-seam producer wiring around `fifa96_match_act.{c,h}`, the helper gaps in `fifa96_loader/fifa96_keeper.c`/`fifa96_action_handlers.c`/`fifa96_outfield.c`. Serialized implementers; parallel read-only recon is the only parallel surface.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan (engine + tests), Ghidra MCP read-only (`/FIFA96.EXE`; the authoritative recon tool — rea MCP only as a secondary read-only view, context7 only if SDL3/CMake docs are needed), SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU183…FU190`; predecessor `docs/superpowers/plans/2026-10-12-fifa96-m2-phase15-census-frontier.md` + its final review; frontier map `docs/ghidra/FU182_census_audit.md` (§7).

## Global Constraints

- Baseline: `make check` **110/110** (sanitized); wired **29/80**; README title **36%**; HEAD `44f8083`. M1 `09b726b7…` (691 lines); M2 `7b4b241a…` (v15 label, 792 lines).
- Census arithmetic (this phase): action non-NULL `28` → up to `+8` (`0x14, 0x22, 0x1A, 0x1C, 0x16, 0x24`, plus conditionally `0x0D`, `03`) and row `0x17` (act machine); phase `1/35` → `+N` producer-live rows the act machine proves. The exact target is whatever the tasks prove — **no hand-set number**; each task recomputes `floor(100 × wired/80)` when its count moves.
- Evidence floor: first-hand bytes/addresses; **fresh `get_xrefs_to` for census claims**; `/FIFA96.EXE` (case-sensitive); +0x100000 rule; **alias traps** (`dword[addr]>>16` = word at `addr+2`; `>>24` = byte at `addr+3`); verify CALL bytes.
- Determinism: null backend source of truth; **M1 never moves**; **T1 is the one tape risk** (the act producer entering the live frame body) — M2 stays v15 if byte-identical (record the check), else additive re-pin **v16** with a written reason + frame diff. T2–T5 are expected tape-dormant (install-live, not tape-dispatched) → no re-pin.
- No fakes; unreachable → numbered legs; single situation entry; ISO-gated tests skip without the ISO.
- **Wired census rule:** if a task changes the wired count, recompute the README title percentage in the same commit per AGENTS.md; keep it consistent with the `docs/ENGINE.md` census line. Status narrative → `docs/PROGRESS.md`.
- Commits `feat/fix/docs/test`; implementers commit and push per dispatch; controller verifies (`make check` + tape `cmp` + tree clean).

---

## Wave R — parallel read-only recon, eight agents (gate: eight FU slices + reports, committed)

R31–R38 run **concurrently** (the user-directed speed-up surface). Each is read-only on the repo: it writes ONLY its new `docs/ghidra/FU18N_*.md` slice and its workspace report; **no git write commands, no edits to existing files, no code**. The controller commits FU-183…FU-190 in one `docs(fu183-fu190)` commit after the wave returns.

### R31 → FU-183: the act-machine producer (the enabler) (unblocks T1)
- Question: derive `FUN_000888FC`'s producer chain — what fills the act slot and drives the per-record phase call (`[rec+0x1C]`, `FUN_0008D098`'s walk — landed by FU-175), the `0x1107EC` act table (FU182-L1) semantics, and the exact engine wiring that gives the phase seam a live phase-row cohort. Derive the **act-4 sit-2..4 formation-order arm** `0x8AB08..0x8AB62` whose stage-0 `0x89740` is row `0x17`'s only producer (FU181-L1). Rank which phase rows become honestly producer-live once the act machine runs.
- Starting windows: FU-175 (the landed seam), FU-182 §1/§5/§9 (the act table `0x1107EC`, phase tail `0x88DC8…`), FU-165 §6 (act-machine legs), FU-149 L12 / FU181-L1 (the sit-2..4 arm), the engine's phase drive (`match_run.c`, `fifa96_match_act.{c,h}`).
- Output: FU-183 + a concrete T1 wiring rec (producer site + which phase rows to count), the honest row-0x17 reachability verdict, or the next hop as a numbered leg.

### R32 → FU-184: row 0x14 celebration/scatter body (unblocks T2)
- Question: derive the row-0x14 body (FU-136 scatter_celebration helpers; OL-9) byte-level — every cell/gate written, the phase-5 arm's team gate, and the goal-record release (stage-2 phase-1 write `0x87C71`, FU173-L2/L3) — onto the ported helpers. The installer is live (engine phase-5 arm `match_run.c:2480`), so rank the minimal honest live port.
- Starting windows: FU-182 §4/§5 (installer act-5 arm `0x8D461` + engine `fifa96_match_run_phase_arm` `match_run.c:2480`), FU-136 (scatter_celebration), FU-173 (the release), OL-9.
- Output: FU-184 + T2 wiring rec, or the body/release remainder as numbered legs.

### R33 → FU-185: row 0x22 sequence/press body + the team-target producer (unblocks T2)
- Question: derive the row-0x22 body (FU-136 sequence_press_event; OL-9) and the **OL-56 team-target producer** (FU-164) that makes its install target real. The installer is live (wired row-07 `fifa96_match_action_07` `handlers.c:1160` → `match_kick_invoke_opponent` → `install(…,0x22,0)` `handlers.c:1122`, gated `out.opponent_invoke` `fifa96_action_handlers.c:1612 diff < 0x55`), but dormant until OL-56 lands. State the honest census verdict under the strict "engine reaches it" test.
- Starting windows: FU-182 §4 (installer bytes `0x816D6`/`0x816DF`; engine row-07 path), FU-136 (sequence_press_event), FU-164/OL-56 (team target), OL-9.
- Output: FU-185 + T2 wiring rec + the honest 0x22 census call (countable now vs. leg-gated on OL-56), or numbered legs.

### R34 → FU-186: keeper chain → rows 0x1A/0x1C (+ row 0x1B legs) (unblocks T3)
- Question: derive the keeper reposition/lunge chain (`FUN_00077EAC`, `FUN_000782D0`, `FUN_00076130`, `FUN_00070C08`) and the rows **0x1A**/**0x1C** bodies whose installers it exposes (`FUN_00077EAC` `0x782xx`, `FUN_0009E420` `0x9E665`, `FUN_00070C08` `0x70DAF`, `FUN_00076130` `0x76291`, `0x80814`; FU-140 OL-34/OL-35), plus the row-0x1B sine/steering legs still open (`0x14E04`/`0x76B28`, FU180-L1/L2/L3). Bounded: what the chain needs for both installs to execute honestly vs. the deep remainder.
- Starting windows: FU-182 §5 (installer bytes), FU-140 §2/§3 + OL-34/OL-35, FU-151 (keeper family), FU-180 (chain legs).
- Output: FU-186 + ranked T3 wiring rec (both bodies now, deep chain as legs).

### R35 → FU-187: row 0x16 fouled-player + phase-0x19 handler + act-3 foul producer (unblocks T4)
- Question: derive the row-0x16 body (FU-148 §16.2) and the **phase-0x19 handler** (`0x8A096 MOV EDX,0x16; 0x8A09B MOV EAX,[0x15888f]; 0x8A0A0 CALL 0x7D9A4`) that installs it, plus the **act-3 foul producer** that makes phase `0x19` reachable once the act machine (T1) runs. State exactly what T4 needs from T1's seam.
- Starting windows: FU-182 §5 (installer bytes `0x8A080`), FU-148 §16.2 (row-0x16 body), FU-150 (fouls/referee), the act seam (FU-175/FU-183).
- Output: FU-187 + T4 wiring rec (0x16 body + the phase-0x19 hook), or the producer remainder as numbered legs.

### R36 → FU-188: row 0x24 + the phase-0x20 handler (unblocks T4)
- Question: derive the row-0x24 body (OL-9) and the **phase-0x20 handler** (`0x8B747` loop `ESI=0x1588A4` 11 rec; `0x8B782` loop `ESI=0x1590D9` — `MOV ECX,1; MOV EDX,0x24; CALL 0x7D9A4`, fresh `disassemble_bytes 0x8B688`, 491 B) that installs it, and the act path that reaches phase `0x20` through T1's seam.
- Starting windows: FU-182 §5 (installer bytes `0x8B688`), OL-9 (row-0x24 body), the act seam (FU-175/FU-183).
- Output: FU-188 + T4 wiring rec, or the body/act-path remainder as numbered legs.

### R37 → FU-189: rows 0x20/0x25 installer hunt + the FU-82 arm tables (unblocks T5)
- Question: resolve FU182-L2 — locate the row-**0x20** installer (the `EDX,0x20` search truncated before the `0x8B…` region) and any real row-**0x25** installer (the only `0x7xxxx MOV EDX,0x25` found, `0x7E460`, is the `0x6E598` anim/sink, not `0x7D9A4`) — re-searching against the FU-82 7-arm tables `0x84ED0`/`0x880B0`, and derive the two bodies (OL-9). **Bounded (designated archaeological risk):** strictly bounded search; if no installer exists, that row stays a numbered leg — do NOT invent one.
- Starting windows: FU-182 §5/§9 (the truncated search + FU182-L2), FU-82 (7-arm tables), OL-9.
- Output: FU-189 + the honest installer verdict per row (installer address + T5 wiring rec, or a leg), or numbered legs.

### R38 → FU-190: census/act/phase audit + the rows-03/0x0D completion path (feeds T5/T6)
- Question: re-verify the count machinery with fresh in-session evidence (the two tables, `get_xrefs_to 0x7D9A4`, the act table `0x1107EC`), state the post-phase-16 % arithmetic, and derive the completion paths for the two ported-but-uncounted rows: **0x0D** (installer row-09 arm 1 `0x80F61`, FU178-L1) and **03** (the live phase-2 path `[rec+0x1C] = 0x6DCC8`, FU177-L1). Rank the phase-16 movers and re-state the blocked/no-move set.
- Starting windows: FU-182 §1/§6/§7/§8, FU-174 §1/§7, FU-177 (row 03 / `0x6DCC8`), FU-178 (row-09 arm 1 / `0x80F61`), `fifa96_match_handlers.c` tables, the ENGINE census line.
- Output: FU-190 with the verified census + the ranked mover/blocked set + the 03/0x0D completion recs.

---

### Task 1: T1 — the act-machine producer + row 0x17 + producer-live phase rows (from R31)

**Files:** per the rec (the act producer site in `src/fifa96_engine/fifa96_match_run.c` and/or `fifa96_match_act.{c,h}`; `include/fifa96_engine/fifa96_match_handlers.h` + `src/fifa96_engine/fifa96_match_handlers.c` row 0x17 slot + the phase-table rows R31 proves producer-live); tests + FU-183 status + `docs/ENGINE.md` + README title.
**Produces:** the act machine's producer runs in the live frame body; `fifa96_match_dispatch_phase` gets a live producer; action row 0x17 dispatches (counted); the phase rows R31 proves producer-live are counted.
**Gate T1:** the producer + row 0x17 + each counted phase row covered by RED/GREEN + mutation-RED tests; a phase-row census claim only if R31 proves it producer-live (else numbered leg, table stays honest). **Tape decision:** replay M2 — byte-identical → no re-pin (record the check), else additive v16 with reason + frame diff. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate (`make check` + tape `cmp` + smoke `p16-t1-*`) → census/README update → commit `feat(engine,fu183): act-machine producer + row 0x17 + live phase rows`.

### Task 2: T2 — celebration + sequence: 0x14 + 0x22 (from R32/R33)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (rows 0x14/0x22 bodies), helper gaps in `fifa96_action_handlers.c`/`fifa96_outfield.c` (scatter_celebration / sequence_press_event), the goal-record release path (`fifa96_match_run.c`); tests + FU-184/FU-185 status + `docs/ENGINE.md` + README title.
**Produces:** rows 0x14 and 0x22 execute end-to-end in the live chain (0x22 counted only if R33 shows the strict "engine reaches it" test passes; if the OL-56 target producer is required and unported, 0x22 is an honest leg and stays uncounted).
**Gate T2:** each body RED/GREEN + mutation-RED; the phase-5 goal arm's live route proven where reachable, else unit fixture + tape-dormancy pin. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p16-t2-*` → census/README update → commit `feat(engine,fu184): rows 0x14 + 0x22 — celebration, sequence/press`.

### Task 3: T3 — keeper chain → rows 0x1A + 0x1C (from R34)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (rows 0x1A/0x1C bodies), `src/fifa96_loader/fifa96_keeper.c` + `fifa96_action_handlers.c` (the `FUN_00077EAC`/`_782D0`/`_76130` chain and row-0x1B sine/steering legs); tests + FU-186 status + `docs/ENGINE.md` + README title.
**Produces:** the keeper chain is ported and both rows 0x1A/0x1C execute; the row-0x1B legs close as far as the chain allows.
**Gate T3:** each body RED/GREEN + mutation-RED; the keeper-chain installs proven; ISO-gated live chain where the keeper route is reachable, else unit fixture + tape-dormancy pin. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p16-t3-*` → census/README update → commit `feat(engine,fu186): keeper chain — rows 0x1A/0x1C + 0x1B legs`.

### Task 4: T4 — face-the-ball + lane: rows 0x16 + 0x24 (from R35/R36)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (rows 0x16/0x24 bodies), the phase-0x19/phase-0x20 handler hooks (`fifa96_match_handlers.c` phase table + the act seam); tests + FU-187/FU-188 status + `docs/ENGINE.md` + README title.
**Produces:** rows 0x16 and 0x24 execute through T1's act-machine phase handlers (phase 0x19 / phase 0x20).
**Gate T4:** each body RED/GREEN + mutation-RED; the phase-handler install path proven; **depends on T1's act seam** — if the act path to phase 0x19/0x20 is not producer-live, the rows are honest legs and stay uncounted. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p16-t4-*` → census/README update → commit `feat(engine,fu187): rows 0x16 + 0x24 — face-the-ball, sequence/lane`.

### Task 5: T5 — installer hunt + 03/0x0D completion (from R37/R38)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (rows 0x20/0x25 iff an installer is found; row 03's handler path; row-09 arm 1 for 0x0D), `fifa96_outfield.c`/`fifa96_action_handlers.c` (the arm-1 helpers); tests + FU-189/FU-190 status + `docs/ENGINE.md` + README title.
**Produces:** `0x0D` counted iff row-09 arm 1 lands (FU178-L1); `03` counted iff the live phase-2 handler `0x6DCC8` lands (FU177-L1); `0x20`/`0x25` counted iff R37 finds a real installer — otherwise honest numbered legs (no invented wiring).
**Gate T5:** each landed row RED/GREEN + mutation-RED; conditional rows either counted or recorded as legs with owners. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p16-t5-*` → census/README update → commit `feat(engine,fu189): installer hunt + rows 03/0x0D completion`.

### Task 6: T6 — acceptance v16 + riders + smoke + docs + final review

**Files:** `tests/test_engine_m2.c` (+ golden iff genuinely moved), `docs/ENGINE.md`, `docs/PROGRESS.md`, `docs/screens/*`.
**Produces:** the v16 strata (or v15-held with recorded reason); the cheap regression pins the T1–T5 reviews surface; the honest smoke record; the remaining legs with owners; the census recomputed against R38's audit.
- [ ] Tests + riders → tape decision → smoke → docs → commit `test(engine): phase-16 acceptance tape v16 and docs` → whole-plan final review (one fix wave max) → close.

---

## Self-Review Notes

- **Coverage:** act machine + row 0x17 + phase rows → R31/T1; celebration 0x14 + sequence 0x22 → R32/R33/T2; keeper chain 0x1A/0x1C (+ 0x1B legs) → R34/T3; face-the-ball 0x16 + lane 0x24 → R35/R36/T4; installer hunt 0x20/0x25 + 03/0x0D completion → R37/R38/T5; acceptance + census → R38/T6.
- **Dependency order:** Wave R (parallel, eight) → T1 → T2 → T3 → T4 → T5 → T6. T1 first (the act producer is the riskiest — it touches the frame body — and T4's phase handlers depend on it); T2/T3 are independent of each other and of T1.
- **Split rule:** R37 (rows 0x20/0x25) is the designated archaeological risk — bounded search + honest legs, the wave does not invent an installer.
- **Determinism:** T1's act producer is the only tape risk (v16 additive iff the frames move); T2–T5 expect no re-pin; M1 never moves.
- **Census:** baseline 29/80 (NOT FU-182's 24/80 — that was the pre-phase-15 snapshot); the target is whatever the tasks prove — no hand-set number; every moving task recomputes the README % + ENGINE line in its own commit.
- **Tooling:** ghidra-mcp is the sole evidence source (AGENTS.md rules); rea MCP only as a secondary read-only view; context7 only if SDL3/CMake docs become relevant.

---

## Landing notes (post-execution)

_(filled in at close-out)_
