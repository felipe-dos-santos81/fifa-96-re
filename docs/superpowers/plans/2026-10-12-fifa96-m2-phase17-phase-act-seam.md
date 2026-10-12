# FIFA 96 M2 phase-17 — the phase/act seam (per-record phase machine + producer machine)

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Wave R runs as **eight parallel READ-ONLY recon subagents** (user-directed, "use 8 agents to speed up the reverse engineer"); implementation tasks stay serialized (one implementer at a time).

**Goal:** move the engine's match phase dispatch from today's single global `fifa96_match_dispatch_phase` onto the **native per-record machine**, then populate it, tape-neutrally. Four pieces: (a) convert the phase dispatch to the native per-record `[rec+0x1C]` walk (`FUN_0008D098`, selector `[0x157A4A]`, 11 records @ `0xB2` stride); (b) port the per-record phase handlers (phase table `0x00–0x15`, the `0x6Dxxx` bodies) and the remaining act-tail handlers (`0x17/0x1A/0x1C/0x1D/0x1E/0x20/0x21/0x22`); (c) port the **producer machine** (situations `0/8/0xB`, the act-4 sit-2..4 formation-order arm, the `[rec+0xC]`/OL-13 metadata source) so the cohort is producer-live; (d) wire action row `03` (`0x6DCC8`). Each landing proven by RED/GREEN + mutation-RED; **tape-neutral** (M2 stays v16 byte-identical, no re-pin).

**Baseline (this plan's, verified at HEAD `3358e0c`):** wired **34/80 = 42%** (31 action + the wired phase rows `0x18`, `0x19`, `0x1F`); `make check` **110/110** (sanitized, `-Wall -Wextra -Werror`); README title **42%**; M1 `09b726b7…` (691 lines); M2 `7b4b241a…` (v16 strata label, 792 lines, byte-identical). FU-190 §9 is the authoritative phase-16 census (`45 UNSUP / 34 OK / 1 NOTF`, sum 80); FU-190 §3's envelope is the projection baseline.

**Architecture:** the native phase table `0x110794[35]` has three regions. **Per-record** `0x00–0x15` (the `0x6Dxxx` family) dispatched per-record by `FUN_0008D098` (selector `[0x157A4A]` → `0x110794` → store `[rec+0x1C]` → `CALL [rec+0x1C]`, 11 records @ `0xB2`). **INT3** `0x16` (native slot `0`, NOTF by design). **Act-tail** `0x17–0x22` (the `0x88xxx` family) dispatched by the act machine `FUN_000888FC` via the dedicated table `0x1107EC` (= `0x110794 + 0x16*4`; act `a` → phase `0x16+a`). The engine already has *half* of each: the act seam (`fifa96_match_act.{c,h}`, landed by phase-16 T1) covers the act-tail path; the per-record path is today modeled by the global `fifa96_match_dispatch_phase`. Phase-17 **unifies the per-record path onto the native walk** and **extends the act seam** across its remaining tail rows — one seam per region, retiring the FU175-L7 double-dispatch class for good. Derivations from FU-175 (the act/phase seam + §7 phase ranking), FU-182 §7 (the frontier), FU-190 (the phase-16 audit + §6 completion paths), FU-177 (row 03), FU-181 (row `0x17`/act-4), FU-186 (the keeper chain), FU-165 §6 (act-machine legs). Consumers: `src/fifa96_engine/fifa96_match_handlers.c` (phase/action tables + bodies), `fifa96_match_run.c` (the frame body + the dispatch seam), `fifa96_match_act.{c,h}` (the act seam), the loader helpers (`fifa96_keeper.c`/`fifa96_action_handlers.c`/`fifa96_outfield.c`). Serialized implementers; parallel read-only recon is the only parallel surface.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan (engine + tests), Ghidra MCP read-only (`/FIFA96.EXE`; the authoritative recon tool — rea MCP only as a secondary read-only view, context7 only if SDL3/CMake docs are needed), SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU191…FU198`; predecessor `docs/superpowers/plans/2026-10-12-fifa96-m2-phase16-act-machine-frontier.md` + its final review; frontier map `docs/ghidra/FU182_census_audit.md` (§7) and `docs/ghidra/FU190_census_audit_phase16.md` (§4/§6).

## Global Constraints

- Baseline: `make check` **110/110** (sanitized); wired **34/80**; README title **42%**; HEAD `3358e0c`. M1 `09b726b7…` (691 lines); M2 `7b4b241a…` (v16 label, 792 lines).
- Census arithmetic (this phase): action non-NULL `31` (baseline; `→32` iff T1 counts row 03, per R3); phase `3/35` → `+N` (the cohort rows the tasks prove producer-live). The exact target is whatever the tasks prove — **no hand-set number**; each task recomputes `floor(100 × wired/80)` when its count moves.
- Evidence floor: first-hand bytes/addresses; **fresh `get_xrefs_to` for census claims**; `/FIFA96.EXE` (case-sensitive); +0x100000 rule; **alias traps** (`dword[addr]>>16` = word at `addr+2`; `>>24` = byte at `addr+3`); verify CALL bytes before trusting a call target.
- **Determinism — the spine:** null backend source of truth; **M1 never moves**; **M2 stays `7b4b241a…` byte-identical** (v16 bytes). The per-record walk is render-invisible by construction; **any presented-frame move is a regression to investigate, not a re-pin** — the task stops and reports. The T7 acceptance adds a **v17 strata label + riders** (the phase-15 v14→v15 precedent: the label tracks the acceptance layer, not the bytes) with the reason recorded; **no re-pin**.
- No fakes; unreachable → numbered legs; single situation entry; ISO-gated tests skip without the ISO.
- **Wired census rule:** if a task changes the wired count, recompute the README title percentage in the same commit per AGENTS.md; keep it consistent with the `docs/ENGINE.md` census line (title + the dispatch sum-to-80 line). Status narrative → `docs/PROGRESS.md`.
- Commits `feat/fix/docs/test`; implementers commit and push per dispatch; controller verifies (`make check` + tape `cmp` + tree clean).

---

## Wave R — parallel read-only recon, eight agents (gate: eight FU slices + reports, committed)

R39–R46 run **concurrently** (the user-directed speed-up surface). Each is read-only on the repo: it writes ONLY its new `docs/ghidra/FU19N_*.md` slice and its workspace report; **no git write commands, no edits to existing files, no code**. The controller commits FU-191…FU-198 in one `docs(fu191-fu198)` commit after the wave returns.

### R39 → FU-191: the per-record phase seam (the enabler) (unblocks T1)
- Question: derive `FUN_0008D098`'s **full** 11-record walk byte-level (the `[0x157A4A]` phase selector's `SAR EAX,0x18` byte decomposition, the `0x110794` index, the `[rec+0x1C]` store, the `CALL dword[ECX+0x1C]`, the `0xB2` stride + `[rec+0x9A]` skip), the **handler signature** the per-record handlers expect (the `param_1`/`param_2`/`param_3` ABI the `0x6Dxxx` bodies read/write — `[rec+0xC]`/`[rec+0x4D]`/`[rec+0x51]`/`[rec+0x55]`), and the engine-conversion rec: exactly how to replace `fifa96_match_dispatch_phase` with the native walk without perturbing the existing act seam or the render.
- Starting windows: FU-175 (the landed act/phase seam + §7), FU-182 §1 (`FUN_0008D098` fresh `0x8D0F7`/`0x8D107`/`0x8D110`/`0x8D163`), FU-190 §1 (`FUN_0006D920`'s `0x6D9A8` selector read + `0x6D9BA` store), FU-177 §3 (`0x6DCC8`'s `[rec+0xC]` source), the engine's `fifa96_match_dispatch_phase` + the frame body in `fifa96_match_run.c`.
- Output: FU-191 + a concrete T1 conversion rec (the walk site, the record loop, the per-record call, the selector source), the handler-ABI contract, and the honest tape-neutrality argument, or the next hop as a numbered leg.

### R40 → FU-192: the per-record phase handlers `0x00–0x15` (unblocks T3)
- Question: derive the **distinct** per-record handler bodies — the fresh phase-table `0x110794[0x00..0x15]` values name the family (`0x6DE34` shared reset used by rows `00/0A/0B/0D/0E/0F`, `0x6DCC8` rows `02/15`, `0x6E1D0` rows `01/12`, `0x6DF4C` row `0C`, `0x6DD9C` row `06`, `0x6DD6C` rows `08/09`, `0x6E05C` row `05`, `0x6E004` row `10`, `0x6E1C8` row `11`, `0x6E244` rows `13/14`; verify fresh) — and, for each, what it writes, what it reads from the record (`[rec+0xC]` metadata, the target block `+0x4D/+0x51/+0x55`), and whether it is **tape-dormant** (render-invisible on the M2 tape's state). Rank which rows become honestly producer-live once the walk runs.
- Starting windows: FU-190 §1 (the fresh phase-table read), FU-177 §1/§3 (the row-03 idle core + `0x6DCC8`), FU-190 §6.2 (the `0x6DCC8` decompile), the per-row `0x6Dxxx` regions (fresh `decompile_function`).
- Output: FU-192 + a per-row T3 wiring rec (body + tape-dormancy verdict) + the ranked producer-live set, or numbered legs.

### R41 → FU-193: the act-tail cohort 1 — phase `0x17`/`0x1A`/`0x1C` (unblocks T4)
- Question: derive the act-tail handlers phase `0x17` (`0x88DC8`, act 1), `0x1A` (`0x89620`, act 4) and `0x1C` (`0x89110`, act 6) byte-level — what each does, its per-record loop / target, and its **producer** (which `CALL 0x888FC` with `EAX=<act>` reaches it; fresh census). State the honest producer-live verdict per row under the strict "engine reaches it" test.
- Starting windows: FU-175 §1/§7 (the act seam + the phase ranking), FU-190 §1 (the `0x1107EC` act table + the phase tail), FU-182 §5, the act-table read `0x1107EC` (fresh), the engine act seam `fifa96_match_act.{c,h}`.
- Output: FU-193 + a T4 wiring rec per phase row (body + producer), or the producer remainder as numbered legs.

### R42 → FU-194: the act-tail cohort 2 — phase `0x1D`/`0x1E`/`0x20` (unblocks T5)
- Question: derive the act-tail handlers phase `0x1D` (`0x89868`, act 7), `0x1E` (`0x8A798`, act 8) and `0x20` (`0x8B688`, act 0xA) — bodies + producers. Note that `0x89868` is the phase-0x1D handler that **contains row-0x20's installer** `0x899a2` (FU-189-L1) — derive whether driving act 7 makes row `0x20` producer-live. Fresh producer census per act.
- Starting windows: FU-175 §1/§7, FU-190 §1, FU-188 (the phase-0x20 handler `0x8B688`), FU-189 §10 (the phase-0x1D/row-0x20 route), the act-table read `0x1107EC` (fresh).
- Output: FU-194 + a T5 wiring rec per phase row + the act-7 → row-`0x20` reachability verdict, or numbered legs.

### R43 → FU-195: the act-tail cohort 3 — phase `0x21`/`0x22` + the act-5/`0x1B` blocked claim (unblocks T6)
- Question: derive the act-tail handlers phase `0x21` (`0x8B874`, act 0xB) and `0x22` (`0x8B900`, act 0xC) — bodies + producers — and **re-verify first-hand** the FU175-L2 / FU-190 §5 claim that act 5 / phase `0x1B` has no static `CALL 0x888FC` invoking `EAX=5` (a fresh 11-xref census of every `0x888FC` caller). Bounded: if `0x1B` truly has no invoker, it stays a numbered leg — do not invent one.
- Starting windows: FU-175 §1/§2/§7 (the act-5/0x1B blocked claim + the `0x888FC` xref census), FU-190 §5, the act-table read `0x1107EC` (fresh).
- Output: FU-195 + a T6 wiring rec for `0x21`/`0x22` + the `0x1B` blocked verdict (leg or reachable), or numbered legs.

### R44 → FU-196: the situation producer machine — situations `0/8/0xB` (unblocks T2)
- Question: derive the **situation machine** — who writes the situation selector, how a situation drives the act (`CALL 0x888FC`), and the reachability of situations `0` (FU-188's row-0x24/phase-0x20 route), `8` (the act-7 producer, FU-189-L1's row-0x20 route) and `0xB` (the already-live row-01 path). Fresh: the situation selector cell + its writers, the situation→act map, and which act-tail/per-record rows flip producer-live once each situation is drivable.
- Starting windows: FU-149 (the situation arms), FU-188 (situation 0), FU-189 §10 (situation 8), FU-175 (the act seam), the engine's `fifa96_match_run_situation` + `fifa96_action_phase_situations`.
- Output: FU-196 + a T2 producer-machine rec (which situations to port, the selector site, the act map) + the per-situation reachability verdict, or numbered legs.

### R45 → FU-197: the act-4 sit-2..4 formation-order arm + the producer audit (unblocks T2)
- Question: derive the **act-4 sit-2..4 formation-order arm** `0x8AB08..0x8AB62` whose stage-0 `0x89740` installs row `0x17` (FU-181-L1) — including its `FUN_0008C974` counter and the `0x14C360`/`0x157BD2` matrices that feed it (FU-183's proven no-op caveat) — and audit the remaining act producers (which `CALL 0x888FC EAX=a` sites exist per act, fresh). Rank row `0x17`'s honest reachability.
- Starting windows: FU-181 (row-`0x17` deps + the sit-2..4 arm), FU-183 (the no-op caveat + the matrix feeders), FU-175 §7, FU-190 §4 (row `0x17` rank 3), the act-4 arm region (fresh `disassemble_bytes`/`decompile_function`).
- Output: FU-197 + a T2/T4 rec for row `0x17` (arm + matrix feeders) + the act-producer audit, or the arm remainder as numbered legs.

### R46 → FU-198: census/audit + row 03 completion (feeds T1/T7)
- Question: re-verify the count machinery with fresh in-session evidence (the two tables `0x1106E0`/`0x110794`, the `0x7D9A4` installer xref count, the act table `0x1107EC`), state the post-phase-17 arithmetic, rank the phase cohort by honest reachability, and derive the completion path for action row `03` (`0x6DCC8` + the per-record `[rec+0x1C]` seam, FU190-L4).
- Starting windows: FU-182 §1/§7, FU-190 §1/§3/§6.2/§9, FU-175 §7 (the phase ranking), `fifa96_match_handlers.c` tables, the ENGINE census line.
- Output: FU-198 with the verified census + the ranked phase-cohort mover/blocked set + the row-03 completion rec.

---

### Task 1: T1 — the per-record phase seam + row 03 (from R39/R46)

**Files:** `src/fifa96_engine/fifa96_match_run.c` (the frame body — replace the global `fifa96_match_dispatch_phase` call with the native per-record walk) and/or `fifa96_match_act.{c,h}` (the seam module); `src/fifa96_engine/fifa96_match_handlers.c` + `include/fifa96_engine/fifa96_match_handlers.h` (the phase-table row-`0x02` slot + the `0x6DCC8` body, row-`03` action slot); tests + FU-191/FU-198 status + `docs/ENGINE.md` + README title.
**Produces:** the native per-record walk runs in the live frame body (11 records, selector `[0x157A4A]` → `0x110794` → `[rec+0x1C]` → call); the `0x6DCC8` handler is ported; action row `03` dispatches it (counted). **Nothing else new is dispatched** in this task, so the tape provably cannot move.
**Gate T1:** the walk + the `0x6DCC8` handler covered by RED/GREEN + mutation-RED; **tape decision:** replay M2 — byte-identical (record the check); any drift is a regression (stop, report) — **not** a re-pin. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate (`make check` + tape `cmp`) → census/README update → commit `feat(engine,fu191): per-record phase seam + row 03`.

### Task 2: T2 — the producer machine (from R44/R45)

**Files:** the situation machine (`fifa96_match_run.c` / a new producer seam) + the act-4 sit-2..4 formation-order arm (`fifa96_match_run.c`/`fifa96_outfield.c`), the `0x14C360`/`0x157BD2` matrices, the `[rec+0xC]`/OL-13 metadata source; tests + FU-196/FU-197 status + `docs/ENGINE.md` + README title.
**Produces:** situations `0/8/0xB` are drivable (the selector + the act map), and the act-4 arm installs row `0x17` — so the act-tail/per-record rows have live producers. Rows whose producer is proven live flip on; rows still producer-gated stay honest numbered legs.
**Gate T2:** each producer RED/GREEN + mutation-RED; the situation selector write + the act-4 arm fixtured; ISO-gated live chain where reachable, else fixture + dormancy pin. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p17-t2-*` → census/README update → commit `feat(engine,fu196): situation/act producer machine + act-4 formation arm`.

### Task 3: T3 — the per-record phase handlers `0x00–0x15` (from R40)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (the per-record phase-handler bodies + the phase-table rows R40 proves live), helper gaps in the loader; tests + FU-192 status + `docs/ENGINE.md` + README title.
**Produces:** the distinct `0x6Dxxx` handlers are ported; every row the walk drives producer-live is counted (the shared `0x6DE34` reset counted once).
**Gate T3:** each body RED/GREEN + mutation-RED; tape-dormancy pinned per row (the walk runs; the handlers are render-invisible on the tape). Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p17-t3-*` → census/README update → commit `feat(engine,fu192): per-record phase handlers 0x00–0x15`.

### Task 4: T4 — act-tail cohort 1: phase `0x17`/`0x1A`/`0x1C` (from R41)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (the `0x88DC8`/`0x89620`/`0x89110` bodies + the phase-table rows), the act seam (`fifa96_match_act.{c,h}`); tests + FU-193 status + `docs/ENGINE.md` + README title.
**Produces:** phase `0x17`/`0x1A`/`0x1C` execute through the act seam; each counted iff R41 proves its producer live (else numbered leg).
**Gate T4:** each body RED/GREEN + mutation-RED; the producer route proven; tape-dormancy pinned. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p17-t4-*` → census/README update → commit `feat(engine,fu193): act-tail phase 0x17/0x1A/0x1C`.

### Task 5: T5 — act-tail cohort 2: phase `0x1D`/`0x1E`/`0x20` (from R42)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (the `0x89868`/`0x8A798`/`0x8B688` bodies + the phase-table rows), the act seam; tests + FU-194 status + `docs/ENGINE.md` + README title.
**Produces:** phase `0x1D`/`0x1E`/`0x20` execute; row `0x20`'s installer (`0x899a2`, inside `0x89868`) flips on iff act 7 is producer-live (R42's verdict); counted per the strict test.
**Gate T5:** each body RED/GREEN + mutation-RED; the act-7 → row-`0x20` route proven or leg'd; tape-dormancy pinned. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p17-t5-*` → census/README update → commit `feat(engine,fu194): act-tail phase 0x1D/0x1E/0x20`.

### Task 6: T6 — act-tail cohort 3: phase `0x21`/`0x22` + the `0x1B` verdict (from R43)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (the `0x8B874`/`0x8B900` bodies + the phase-table rows), the act seam; tests + FU-195 status + `docs/ENGINE.md` + README title.
**Produces:** phase `0x21`/`0x22` execute (counted iff producer-live); phase `0x1B`/act 5 is either wired (if R43 finds an invoker) or recorded as an honest leg with owner FU175-L2.
**Gate T6:** each body RED/GREEN + mutation-RED; the `0x1B` verdict pinned; tape-dormancy pinned. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p17-t6-*` → census/README update → commit `feat(engine,fu195): act-tail phase 0x21/0x22 + 0x1B verdict`.

### Task 7: T7 — acceptance v17 + riders + smoke + docs + final review

**Files:** `tests/test_engine_m2.c` (+ golden iff genuinely moved — expected not), `docs/ENGINE.md`, `docs/PROGRESS.md`, `docs/screens/*`.
**Produces:** the v17 strata (strata-only label; M2 bytes byte-identical — no re-pin); the seam rider (mutation-RED: a silent re-seam/double-run/`[rec+0x1C]` slip aborts the tape); the cheap regression pins the T1–T6 reviews surface; the honest smoke record; the remaining legs with owners; the census recomputed against R46's audit.
- [ ] Tests + riders → tape decision (byte-identical; record the check) → smoke → docs → commit `test(engine): phase-17 acceptance tape v17 and docs` → whole-plan final review (one fix wave max) → close.

---

## Self-Review Notes

- **Coverage:** per-record seam → R39/T1; per-record handlers `0x00–0x15` → R40/T3; act-tail `0x17`/`0x1A`/`0x1C` → R41/T4; `0x1D`/`0x1E`/`0x20` → R42/T5; `0x21`/`0x22` + `0x1B` → R43/T6; the producer machine (situations `0/8/0xB`) → R44/T2; the act-4 formation-order arm (row `0x17`) → R45/T2; census + row 03 → R46/T1/T7.
- **Dependency order:** Wave R (parallel, eight) → T1 → T2 → T3 → T4 → T5 → T6 → T7. T1 first (the seam is the architectural spine and the phase's one tape risk — landed with nothing new to dispatch); T2 second (the producer machine, so the cohort lands already-countable — the phase-16 T1 producer-first ordering); T3–T6 are the cohort bodies.
- **Split rule:** if one act-tail cluster's probe exceeds ~4 tasks, split it into its own plan (M2 spec §12 rule). R43 (the act-5/`0x1B` blocked claim) and R44/R45 (producer reachability) are the designated archaeological-risk, bounded-search items — the wave does not invent a producer.
- **Determinism:** the seam conversion is the phase's one frame-body touch — landed with nothing new dispatched (T1), so the tape provably cannot move; T2–T6 add producers/handlers whose writes must be render-invisible (each `cmp`-verified; any drift = regression, not a re-pin). M1 never moves; M2 stays `7b4b241a…`.
- **Census:** baseline 34/80 (31 action + phase `0x18`/`0x19`/`0x1F`); the target is whatever the tasks prove — no hand-set number; every moving task recomputes the README % + ENGINE line in its own commit.
- **Tooling:** ghidra-mcp is the sole evidence source (AGENTS.md rules); rea MCP only as a secondary read-only view; context7 only if SDL3/CMake docs become relevant.

---

## Landing notes (post-execution)

_(filled in at close-out)_
