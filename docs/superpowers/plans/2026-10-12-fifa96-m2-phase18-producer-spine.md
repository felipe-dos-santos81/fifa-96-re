# FIFA 96 M2 phase-18 — the producer spine (formation/line-up setup + situation-machine live + act-tail producers)

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Wave R runs as **eight parallel READ-ONLY recon subagents** (user-directed, "use 8 agents to speed up the reverse engineer"); implementation tasks stay serialized (one implementer at a time).

**Goal:** move the phase/act cohort from *dispatched-but-gated / producer-leg'd* to **producer-live** by porting the three producer clusters the last three phases deliberately deferred — (a) the **formation/line-up setup** (`FUN_00011620`/`FUN_00012010`: the per-record OL-13 metadata `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` + the `0x14C360`/`0x157BD2` desired-order matrices); (b) the **situation-machine live wiring** (`period_setup_live`, FU196-L8, + the extra-time flag OL-85); (c) the **act-tail producers** (the Tier-A sit-1/contact chains). Each producer proven by RED/GREEN + mutation-RED; **tape posture case-by-case** — each producer's tape-reachability is classified first-hand by the recon wave and the controller rules per producer (re-pin M2 only if a landing provably moves a presented frame *and* the move is native-faithful; M1 never moves).

**Baseline (this plan's, verified at HEAD `357d80b`):** wired **48/80 = 60%** (31 action + 17 phase); `make check` **110/110** (sanitized, `-Wall -Wextra -Werror`); README title **60%**; M1 `09b726b7…` (691 lines); M2 `7b4b241a…` (v17 strata label, 792 lines, byte-identical). `FU198 §3/§4` is the authoritative phase-17 census (`31 UNSUP / 48 OK / 1 NOTF`, sum 80) and its envelope is the projection baseline (the producer spine could reach **~66/80 = 82%**; `0x1B` never fires).

**Architecture:** the engine has **bodies everywhere, producers nowhere**. The per-record phase family (`0x00–0x15`) and the act-tail family (`0x17–0x22`) are ported, but their producer machines are not — so 7 per-record rows are **dispatched-but-gated no-ops** (their `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` metadata is NULL) and the act-tail production rows (`0x17`/`0x1A`/`0x1C`/`0x1D`/`0x1E`/`0x20`/`0x22`) sit behind unported producers. Three clusters supply them: **Cluster 1** the formation/line-up setup (the keystone — writes the per-record metadata **and** the desired-order matrices); **Cluster 2** the situation machine's live arm + the extra-time flag; **Cluster 3** the act-tail sit-1/contact producers. Capture order **1 → 2 → 3** (keystone-first): each cluster closes fully (bodies live, census moved, tape verdict recorded) before the next. Consumers: `src/fifa96_engine/fifa96_match_run.c` (the producer seam + the frame body), `fifa96_match_handlers.c` (the per-record bodies + the tables), `fifa96_match_act.{c,h}` (the act seam), the loader helpers (the formation/roster parser, extending `fifa96_match_entities_seed_formation`). Serialized implementers; parallel read-only recon is the only parallel surface.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan (engine + tests), Ghidra MCP read-only (`/FIFA96.EXE`; the authoritative recon tool — rea MCP only as a secondary read-only view, context7 only if SDL3/CMake docs are needed), SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU199…FU206`; predecessor `docs/superpowers/plans/2026-10-12-fifa96-m2-phase17-phase-act-seam.md` + its final review; frontier map `docs/ghidra/FU198_census_audit_phase17.md` (§4 the ranked cohort) and `docs/ghidra/FU190_census_audit_phase16.md` (§3/§4/§6).

## Global Constraints

- Baseline: `make check` **110/110** (sanitized); wired **48/80**; README title **60%**; HEAD `357d80b`. M1 `09b726b7…` (691 lines); M2 `7b4b241a…` (v17 label, 792 lines).
- Census arithmetic (this phase): action non-NULL `31` + phase `17` = `48`; `+N` for the rows the tasks prove producer-live. The exact target is whatever the tasks prove — **no hand-set number**; each task recomputes `floor(100 × wired/80)` when its count moves.
- Evidence floor: first-hand bytes/addresses; **fresh `get_xrefs_to` for census claims**; `/FIFA96.EXE` (case-sensitive); +0x100000 rule; **alias traps** (`dword[addr]>>16` = word at `addr+2`; `>>24` = byte at `addr+3`); verify CALL bytes before trusting a call target.
- **Determinism — the spine:** null backend source of truth; **M1 never moves**; M2 starts at the v17 bytes `7b4b241a…`. **Tape posture is case-by-case:** each task classifies its producer's tape-reachability first-hand and records the verdict; a landing that provably moves a presented frame *and* is native-faithful re-pins M2 with the written reason + frame diff; otherwise the producer lands gated/dormant and M2 stays byte-identical. **Any unexplained presented-frame move is a regression to investigate, not a re-pin.**
- No fakes; unreachable → numbered legs; single situation entry; ISO-gated tests skip without the ISO.
- **Wired census rule:** if a task changes the wired count, recompute the README title percentage in the same commit per AGENTS.md; keep it consistent with the `docs/ENGINE.md` census line (title + the dispatch sum-to-80 line). Status narrative → `docs/PROGRESS.md`.
- Commits `feat/fix/docs/test`; implementers commit and push per dispatch; controller verifies (`make check` + tape `cmp` + tree clean).

---

## Wave R — parallel read-only recon, eight agents (gate: eight FU slices + reports, committed)

R47–R54 run **concurrently** (the user-directed speed-up surface). Each is read-only on the repo: it writes ONLY its new `docs/ghidra/FU20N_*.md` slice and its workspace report; **no git write commands, no edits to existing files, no code**. The controller commits FU-199…FU-206 in one `docs(fu199-fu206)` commit after the wave returns.

### R47 → FU-199: the formation/line-up setup (the keystone) (unblocks T1)
- Question: derive `FUN_00011620`/`FUN_00012010` (the line-up/formation setup) byte-level — what it reads (the formation-file family `352ko.fmt` + the `[0x14C1E4]`/`[0x14C1E5]` id + the roster), **what per-record metadata it writes** (`[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` — the OL-13 source, FU192-L1) and the `0x14C360`/`0x157BD2` desired-order matrices (FU197-L4). Fresh xref/caller census: exactly which engine/native call reaches it (the match begin? a new setup step?), and where its outputs must land in the engine's record/team model.
- Starting windows: FU197-L4 (the formation setup leg), FU192-L1 (the OL-13 metadata source), FU183/FU181 (the order matrices), the engine's `fifa96_match_entities_seed_formation` + the `formation_order`/`formation_current` fields, the `FUN_00011620`/`FUN_00012010` regions (fresh `decompile_function`).
- Output: FU-199 + a concrete T1 conversion rec (the setup site, the metadata writes, the matrix fills) + the per-write tape-dormancy argument, or the next hop as a numbered leg.

### R48 → FU-200: the OL-13 metadata writers/consumers + the 7 gated rows (unblocks T1/T5)
- Question: for each of the 7 dispatched-but-gated per-record rows (`0x03`/`0x04`/`0x06`/`0x07`/`0x08`/`0x09`/`0x15`) + phase `0x02`, derive the exact metadata read (`[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` via the `phase_meta` NULL gate) and prove whether the R47 setup makes each row **genuinely live**; state whether the R47 metadata write is **render-invisible on the M2 tape's state** (the tape-dormancy verdict).
- Starting windows: FU192 (the per-row bodies + the gated set), FU191 (the walk), the `fifa96_match_handlers.c` gate sites (`:4594/:4603/:4668/:4729/:4748`), the R47 output.
- Output: FU-200 + the T1/T5 un-gate rec per row + the tape-dormancy verdict per row, or numbered legs.

### R49 → FU-201: the order matrix + the act-4 formation arm (rows `0x17`/`0x1A`) (unblocks T2)
- Question: derive the `0x14C360`/`0x157BD2` desired-order matrices (the FU197-L4 producer) and the act-4 sit-2..4 arm `0x8AB08..0x8AB62` (the two `FUN_0008C974` sorts, the `[0x157A4C] != 1 && ([0x157B8E]|[0x157B8F])` gate) — does the R47 matrix fill make the arm live (un-gating `formation_order_ok`)? Trace the arm's row-`0x17` install (`0x89740`) and the row-`0x1A` producer (`0x89620`).
- Starting windows: FU197 (the arm + gate), FU193 (rows `0x17`/`0x1A` bodies + the double-run legs), FU181-L1, the `formation_order_*` engine fields.
- Output: FU-201 + the T2 wiring rec for rows `0x17`/`0x1A` + the arm-live verdict, or numbered legs.

### R50 → FU-202: the situation-machine live wiring + the extra-time flag (unblocks T3)
- Question: derive the situation machine's live arm — the `fifa96_match_run_period_setup` / `period_setup_live` un-gating (FU196-L8), the extra-time flag producer (OL-85, `0x8B2AC`/`0x8B2C1` / the `[0x157AC0]` writer), and the situations `0`/`1`/`8` reachability once live. Which act-tail rows flip live (Tier B `0x1D`/`0x1E`/`0x20`)? Fresh writer census. **Case-by-case tape verdict:** does wiring `period_setup_live` at `begin` move presented frames (the tape's match-start runs `begin`)?
- Starting windows: FU196 (the situation machine + L8), FU194 (the Tier-B bodies + the coupled row-`0x20` installer), FU188/FU189 (situations 0/8), FU143/OL-85 (extra-time), the engine `fifa96_match_run_period_setup`/`period_setup_live`.
- Output: FU-202 + the T3 wiring rec + the per-producer tape verdict + the extra-time leg, or numbered legs.

### R51 → FU-203: the act-tail Tier-A producers (`0x17` sit-1 / `0x1C` contact) (unblocks T4)
- Question: derive the act-1 sit-1 producer (row `0x17`'s kickoff/act-1 entry `0x88DC8` + the sit-1 arm `0x8ABCF`) and the act-6 contact/referee chain (row `0x1C` `0x89110`) — do the live engine paths reach them (the derived kickoff entry vs the ad-hoc `fifa96_match_run_kickoff_entry` double-run, FU193-L1/L2; the contact chain, FU193-L4)? Fresh producer census per act.
- Starting windows: FU193 (rows `0x17`/`0x1C` bodies + the double-run legs), FU175 §7 (the ranking), FU-170/FU-149 (sit-1/contact), the act seam `fifa96_match_act.{c,h}`.
- Output: FU-203 + the T4 wiring rec per row + the double-run verdict (leg or live), or numbered legs.

### R52 → FU-204: the `0x1A` ladder + row-`0x0D` + the row-`0x24`/situation-0 route (unblocks T2/T5)
- Question: derive the row-`0x1A` `FUN_00077EAC` ladder (FU186-L5) + the FU193-L3 act-4 producer; the row-`0x0D` producer (row-09 arm 1 `0x80F61`, FU178-L1) and its gate producers (`[0x15877C]` FU178-L6, `[rec+0x3D]`, `[rec+0x24]`, `[[rec+4]+0x13]`); and the row-`0x24`/phase-`0x20` situation-0 route (FU188-L1). Which become live under R47/R50?
- Starting windows: FU186 (the ladder), FU178 (row-09 arm 1), FU188 (situation 0), FU193-L3, FU189.
- Output: FU-204 + the T2/T3/T5 wiring recs + the honest legs.

### R53 → FU-205: the gated-no-op verification + the coupled row-`0x20` installer (unblocks T5)
- Question: re-verify first-hand the 7 gated rows' gate truth-table (the `phase_meta` NULL path — the exact disjuncts) and the coupled phase-`0x1D`/row-`0x20` installer `0x899A2` route (`0x899A2 = MOV EDX,0x20; MOV EAX,[ESP+8]; CALL 0x7D9A4`); state whether R50's live wiring makes the row-`0x20` installer fire, and pin the remaining gate cells.
- Starting windows: FU194 (the coupling), FU192 (the gated rows), FU189 §10 (the row-`0x20` route), the `fifa96_match_handlers.c` gate sites (`read_memory`/`disassemble_bytes`/`decompile_function`).
- Output: FU-205 + the T5 rec + the census delta, or numbered legs.

### R54 → FU-206: census/audit + the phase-18 envelope + the per-producer tape verdict (feeds T1–T6)
- Question: re-verify the count machinery with fresh in-session evidence (the two tables `0x1106E0`/`0x110794`, the `0x7D9A4` installer xref count, the act table `0x1107EC`), state the post-phase-18 arithmetic, rank the remaining legs by honest reachability, and **compile the per-producer tape-reachability register** (the case-by-case posture's decision input).
- Starting windows: FU198 §3/§4/§5, FU190 §1/§3/§6, the engine `fifa96_match_handlers.c` tables, the ENGINE census line.
- Output: FU-206 with the verified census + the mover/leg set + the tape-verdict register.

---

### Task 1: T1 — the formation/line-up setup + OL-13 metadata (from R47/R48)

**Files:** `src/fifa96_engine/fifa96_match_run.c` (the setup seam / the frame body) and/or a new loader helper (the formation/roster parser), extending `fifa96_match_entities_seed_formation`; `src/fifa96_engine/fifa96_match_handlers.c` + `include/fifa96_engine/fifa96_match_handlers.h` (the per-record bodies' metadata reads) ; tests + FU-199/FU-200 status + `docs/ENGINE.md` + README title.
**Produces:** the formation/line-up setup runs on the live begin path and writes the per-record metadata (`[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]`) + the desired-order matrices, so the 7 dispatched-but-gated per-record rows (and phase `0x02`) perform the native work. Rows whose metadata R47/R48 proves live flip to genuinely-live; rows still gated stay honest numbered legs. **Tape posture case-by-case:** R47/R48's verdict decides — land live if its writes are render-invisible on the tape, else gate and leg.
**Gate T1:** the setup + the metadata reads covered by RED/GREEN + mutation-RED; **tape decision:** replay M2 — byte-identical, OR a re-pin with the written reason + frame diff iff the move is proven native-faithful (the controller rules on R47/R48's verdict); M1 never moves. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate (`make check` + tape `cmp`) → census/README update → commit `feat(engine,fu199): the formation/line-up setup + OL-13 metadata`.

### Task 2: T2 — the order matrix + the act-4 formation arm (rows `0x17`/`0x1A`) (from R49/R52)

**Files:** `src/fifa96_engine/fifa96_match_run.c` (the act-4 arm + the `FUN_0008C974` sorts over the order matrices), the loader helpers for the matrix fill; `fifa96_match_handlers.c` (the row-`0x17`/`0x1A` slots); tests + FU-201/FU-204 status + `docs/ENGINE.md` + README title.
**Produces:** the desired-order matrices are filled by T1's setup, the act-4 sit-2..4 formation arm runs (`formation_order_ok` un-gated), and rows `0x17`/`0x1A` install and dispatch — counted iff R49/R52 proves the arm live; else honest legs (no fake producer).
**Gate T2:** the arm + the sorts RED/GREEN + mutation-RED; the row install route proven or leg'd; tape posture case-by-case (per R49/R52). Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p18-t2-*` → census/README update → commit `feat(engine,fu201): the order matrix + the act-4 formation arm (rows 0x17/0x1A)`.

### Task 3: T3 — the situation-machine live wiring + the extra-time flag (from R50/R52/R53)

**Files:** `src/fifa96_engine/fifa96_match_run.c` (the situation-machine live arm + `period_setup_live` + the extra-time flag), the act seam (`fifa96_match_act.{c,h}`); `fifa96_match_handlers.c` (the Tier-B phase-table rows `0x1D`/`0x1E`/`0x20` + the coupled row-`0x20` installer); tests + FU-202/FU-205 status + `docs/ENGINE.md` + README title.
**Produces:** situations `0`/`1`/`8` are drivable on the live path and the Tier-B act-tail rows (`0x1D`/`0x1E`/`0x20`) + the coupled row `0x20` become reachable — each counted iff R50/R53 proves its producer live. The extra-time flag (OL-85) is either produced (if R50 finds the writer live) or recorded as a numbered leg.
**Gate T3:** each body RED/GREEN + mutation-RED; the situation selector + the extra-time writer fixtured; the per-producer tape verdict applied (R50); ISO-gated live chain where reachable, else fixture + dormancy pin. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p18-t3-*` → census/README update → commit `feat(engine,fu202): situation-machine live wiring + extra-time`.

### Task 4: T4 — the act-tail Tier-A producers (rows `0x17`/`0x1C`) (from R51)

**Files:** the act seam (`fifa96_match_act.{c,h}`) + `fifa96_match_run.c` (the sit-1 / contact producer wiring); `fifa96_match_handlers.c` (the rows' install); tests + FU-203 status + `docs/ENGINE.md` + README title.
**Produces:** the act-1 sit-1 producer (row `0x17`) and the act-6 contact chain (row `0x1C`) reach the rows live — each counted iff R51 proves its producer live and the double-run is resolved (the ad-hoc `fifa96_match_run_kickoff_entry` / the offside-machine conflict retired or leg'd).
**Gate T4:** each producer route RED/GREEN + mutation-RED; the double-run verdict pinned (R51); tape posture case-by-case. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p18-t4-*` → census/README update → commit `feat(engine,fu203): act-tail Tier-A producers (0x17/0x1C)`.

### Task 5: T5 — the remaining producer-gated bound + the gated-no-op deep verification (from R48/R52/R53)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (the remaining body slots), helper gaps in the loader; tests + FU-200/FU-204/FU-205 status + `docs/ENGINE.md` + README title.
**Produces:** every row R48/R52/R53 proves live is wired/counted; every row still frozen is recorded as an honest numbered leg with its owner (row `0x0D` row-09 arm 1 FU178-L1; row `0x24`/phase `0x20` FU188-L1; the `0x1A` ladder FU186-L5; the gated-no-op gates FU192). No fake producer.
**Gate T5:** each body RED/GREEN + mutation-RED; the gated-row gate truth-table pinned; tape-dormancy pinned per row. Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p18-t5-*` → census/README update → commit `feat(engine,fu205): remaining producer-gated bound + gated-no-op verification`.

### Task 6: T6 — acceptance v18 + riders + smoke + docs + final review

**Files:** `tests/test_engine_m2.c` (+ golden iff genuinely moved — per the case-by-case verdict), `docs/ENGINE.md`, `docs/PROGRESS.md`, `docs/screens/*`.
**Produces:** the v18 strata (a re-pin iff earned with the written reason + frame diff, else the strata-only label — the phase-15 v14→v15 precedent); the producer riders (mutation-RED: a silent producer re-gate / double-run / metadata slip aborts the tape); the cheap regression pins the T1–T5 reviews surface; the honest smoke record; the remaining legs with owners; the census recomputed against R54's audit.
- [ ] Tests + riders → tape decision (byte-identical or an earned re-pin; record it) → smoke → docs → commit `test(engine): phase-18 acceptance tape v18 and docs` → whole-plan final review (one fix wave max) → close.

---

## Self-Review Notes

- **Coverage:** the formation/line-up setup → R47/T1; the OL-13 metadata + the 7 gated rows → R48/T1/T5; the order matrix + the act-4 arm (rows `0x17`/`0x1A`) → R49/T2; the situation-machine live wiring + extra-time → R50/T3; the act-tail Tier-A producers → R51/T4; the `0x1A` ladder + row `0x0D` + row `0x24`/situation-0 → R52/T2/T5; the gated-no-op verification + the row-`0x20` installer → R53/T5; census + envelope + tape verdict → R54/T6.
- **Dependency order:** Wave R (parallel, eight) → T1 → T2 → T3 → T4 → T5 → T6. **T1 first** (the keystone — the metadata/order producer everything else consumes); T2 next (the order matrix + the act-4 arm, which T1's matrices feed); T3 (the situation machine, which needs the Tier-B bodies T5 of phase-17 landed); T4 (the act-tail producers, independent of T1–T3 but serialized); T5 (the residual bound); T6 (acceptance).
- **Split rule:** if the formation/line-up setup (T1) probe exceeds ~4 tasks, split it into its own plan (M2 spec §12 rule). R50/R51/R52 are the designated archaeological-risk, bounded-search items — the wave does not invent a producer; a producer with no live route is a numbered leg, never faked.
- **Determinism:** the tape posture is **case-by-case** — every producer is classified first-hand (R47/R50/R54); a landing lands live iff its writes are render-invisible on the tape or the move is proven native-faithful (then the controller authorises an M2 re-pin with the written reason + frame diff). M1 never moves. Any unexplained presented-frame move = regression, not a re-pin.
- **Census:** baseline `48/80` (31 action + 17 phase); the target is whatever the tasks prove — no hand-set number; every moving task recomputes the README % + ENGINE line in its own commit. The `FU198 §4` absolute ceiling is `66/80 = 82%` (`0x1B` never fires).
- **Tooling:** ghidra-mcp is the sole evidence source (AGENTS.md rules); rea MCP only as a secondary read-only view; context7 only if SDL3/CMake docs become relevant.

## Landing notes (post-execution)

_Filled at acceptance (T6)._

**Outcome:** the phase-18 spine landed **producer-live** (not the plan's
literal producers — see the R14/R15 deviations). Census **`48/80 = 60%` →
`51/80 = 63%`** (`+3`: action row `0x24` + phase rows `0x20` and `0x17`); the
FU-198 §4 absolute ceiling (`66/80 = 82%`) was not reached and is not asserted
(the plan's "no hand-set number" rule). `make check` **110/110** (ASan/UBSan,
`-Wall -Wextra -Werror`); README title + `docs/ENGINE.md` title + the dispatch
sum-to-80 line + `docs/PROGRESS.md` + `AGENTS.md:90` all consistent at `51/80 =
63%` (`28 UNSUP / 51 OK / 1 NOTF`, sum 80).

**Landed vs planned (per task):**

- **T1 (FU-199/FU-200) — the OL-13 metadata bind.** Planned (R15) as the
  `FUN_0006D920` port; **landed**: `fifa96_match_entities_bind_record_meta` + the
  `match_run_record_meta_bind` loader, placed as the **last** begin step (a
  documented FU-199 §7.2 erratum — the native bind precedes the derived screen
  install, so a pre-install placement moved the tape from frame 12). The 8
  gated rows now perform native work; **census unchanged** (they were already
  counted). Tape byte-identical at `7b4b241a…`; fix round 1 made the bind
  **atomic** (the partial-bind UAF).
- **T2 (FU-201/FU-204) — the order matrix + act-4 arm.** Planned as a
  matrix/arm port; **landed docs-only**: R49/R15 proved the arm is a permutation
  residual (un-gating on the fill alone is a no-op or a **fake producer**), so
  rows `0x17`/phase `0x1A` stay NULL/uncounted (**+0**) and the matrices are a
  staged leg (**FU201-L4**). FU-204's "act-4 flips live under R47" corrected as a
  quoted erratum.
- **T3 (FU-202) — the situation-machine live wiring + extra-time.** Planned
  gated (+0, option A) then re-ruled to the structural rework (R15 option B,
  user-accepted); **landed `+2`**: `period_setup_live` runs the match-init
  situation-0 producer after the begin-tail act reset, so situation 0 → act `0xA`
  → phase `0x20` → row `0x24` become live; **M2 re-pinned once**
  (`7b4b241a… → 441415fc…`, the earned cutscene move). The extra-time flag
  (OL-85) found no engine writer → leg **FU202-L1**. Fix round 1: the stale
  "reaches phase 2 naturally" claims corrected + the misleading test name.
- **T4 (FU-203) — the act-tail Tier-A producers.** Re-ruled (R19) to attempt the
  act-1 route after T3's cutscene gap surfaced; **landed `+1`**: phase `0x17`
  registered (the native act-1 route), the begin derived kickoff **re-expressed**
  as `fifa96_match_act_dispatch(mr,1,0,1)`; the natural probe again reaches phase
  2 (step 1802). Row `0x1C` (act-6/offside) leg'd **FU203-L3/L4**; M2 stayed
  `441415fc…` (the body fires past the scripted window). Fix round 1: the
  stale act-1-unported comment + the T3-section errata pointers.
- **T5 (FU-205) — the remaining producer-gated bound.** **Landed `+0`**
  (verification + legs): the gated-row gate truth-table pinned
  (`test_gated_rows_gate_truth_table`); the residual frozen rows leg'd (row
  `0x0D` FU178-L1, row `0x1A` FU186-L5, phase-`0x1D`/row-`0x20` FU205-L2, phase
  `0x1E` FU194-L2); the pre-T3 "row `0x24`/phase `0x20` frozen" note corrected.
- **T6 (FU-206) — acceptance.** **Landed**: the **v18** strata label +
  `run_phase18_riders` (the three silent-producer protections, each
  mutation-RED); the census recomputation against R54's fresh audit; the
  `AGENTS.md:90` sweep; the honest smoke record; the leg inventory; this landing
  notes section. Tape decision: the T3 re-pin documented, **no further re-pin**;
  M1 unmoved.

**Tape posture (case-by-case, the plan's determinism rule).** Exactly one
presented-frame move in the whole spine — T3's situation-0 kickoff cutscene
installs code `0x24` on all 22 records at `begin`, and row `0x24`'s `pos_x =
0x9F0` is render-consumed (`match_run_scene_stage` reads `pos_x`). That move is
**native-faithful** (FU-202 §6 rec (b): the re-placed `FUN_0004B02C(1)`
producer), so the controller authorised the single M2 re-pin
(`7b4b241a… → 441415fc…`; frame 12 first diff, 781/792 lines hash-only, **0
`state=` suffix diffs**) with the written reason + frame diff. T1/T2/T4/T5/T6
moved no presented frame (the phase-`0x17` body fires only past the scripted
tape's forced window; the T1 bind and the T5 verification are render-invisible by
measurement). **M1 `09b726b7…` never moved.** Any unexplained drift is a
regression to investigate, never a re-pin.

**Deviations from the plan (recorded, not hidden):**

- **R14/R15** — the plan's keystone premise (`FUN_00011620` writes the OL-13
  metadata) is falsified (it is the front-end menu roster builder); the bind is
  `FUN_0006D920`. The user chose **option B** (the honest phase A + the
  structural situation-machine rework) re-scoping T1/T2/T3.
- **R19** — T3 landed the situation-0 cutscene but left the match holding in the
  cutscene (the act-1 route unported); T4 was therefore re-scoped to close the
  gap (restoring natural phase 2). **R20** — T4 closed it, superseding T3's R17
  re-baseline.
- **T3/T4 partial rework** — the ad-hoc derived kickoff was **re-placed** (T3)
  then **re-expressed** (T4), not fully retired; the frame-body derived
  `global_5882a` arm is retained idempotently (leg **FU203-L2**).
- **T3's 50-site test isolation** — the now-live situation-0 machine pre-empts
  the derived kickoff chain on any granted frame, so the derived-path fixtures
  clear the armed act; documented per-site, no production behaviour change.

**Remaining legs (numbered, with owners) — no fake producer:**

| leg | subject | owner |
|---|---|---|
| FU199-L1 / FU200-L3 / FU200-L4 / FU200-L6 / FU200-L7 / FU199-L7 | the `.fmt` blob field semantics; the index selectors `[0x1577DE]`/…; the `0x6DE44` DX<0 cell map; the `form*6+3` slot; the `[rec+0x90]` role byte | T1 |
| FU201-L1 / FU201-L4 | the runtime non-permutation trigger (no writer); the desired-order matrix fill | T2 |
| FU202-L1 / FU202-L2 / FU202-L3 | the OL-85 extra-time flag writer (no engine cell); the native act-1 route (closed by T4 — FU203-L2 remains); the act-`0xA` invoke-now cadence | T3 |
| FU203-L2 / FU203-L3 / FU203-L4 | the frame-body derived `global_5882a` arm (idempotent, kept); the act-6/offside contact chain (row `0x1C`, double-model) | T4 |
| FU205-L2 / FU188-L2 | the coupled phase-`0x1D`/row-`0x20` pair (extra-time flag + arm-0 `0x1587D4`); the phase-`0x20` `0x3E8` wait sink | T3/T5 |
| FU194-L2 | phase `0x1E` (act-8 compressed fallback; leave NULL) | T5 |
| FU178-L1 / FU186-L5 / FU175-L2 | row `0x0D` (row-09 arm-1 installer `0x80F61`); row `0x1A` (the keeper CPU ladder `FUN_00077EAC`); phase `0x1B` (no invoker) | T5 |
| FU206-L1..L5 | the phase-`0x20`/row-`0x24` coupling is the one true tape risk (§5); the action-side ceiling > FU-198's 66; the 7 blocked phase rows; the row-`0x17` T2/T4 double-count risk; the per-record reach set | R54/T6 |

**Final review:** one fix wave max; clean (`make check` 110/110, tape `cmp`
clean, tree clean). Spec authority unchanged:
`docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`.

