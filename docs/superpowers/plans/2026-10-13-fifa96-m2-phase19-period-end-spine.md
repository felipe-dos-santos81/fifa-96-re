# FIFA 96 M2 phase-19 — the period-end spine (the extra-time / act-7-8 live producer chain)

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Wave R (R55–R62) has **already run** as eight parallel READ-ONLY recon subagents; the slices `docs/ghidra/FU207…FU214` are committed at `30e57d8`. Implementation tasks stay serialized (one implementer at a time).

**Goal:** un-gate the already-ported **extra-time / act-7-8 bodies** by porting their **live producer chain**, from the top down: the per-frame clock's **period completion** → the score tie-break / **OL-85 extra-time flag** (`[0x157AC0]`) + the **period-index roll** (`[0x157AC2]`, the period-4 skip) → the chooser (`FUN_0008B9CC` → act `0xC`/`0xB`) → the **situation-8** selection (`[0x157AC2]==4` OR bit-8 of `[0x14C1D4]|[0x14C1D6]`) → act `7`/phase `0x1D` → the coupled action row `0x20`. The engine already has **bodies everywhere, producers nowhere**: `fifa96_match_action_phase_1D` (`match_run.c:5998`) and `fifa96_match_action_phase_22` (`:6158`) are ported but their table slots are `NULL` and the situation-8 / extra-time inputs are inert (`extra_time` hardwired `0u` at `match_run.c:2583`; `global_157ac2` only ever `0`/`4`). Each producer proven by RED/GREEN + mutation-RED; **any row counts only when a ported path installs + dispatches it** — a producer with no live route stays a numbered leg, never faked. **Tape posture case-by-case** (recon verdict: **no candidate moves M2** ⇒ M2 stays byte-identical; M1 never moves).

**Baseline (this plan's, verified at HEAD `30e57d8`):** wired **51/80 = 63%** (32 action + 19 phase; dispatch `51 OK / 28 UNSUP / 1 NOTF`, sum 80); `make check` **110/110** (sanitized, `-Wall -Wextra -Werror`); README title **63%**; M1 `09b726b7…` (691 lines); M2 `441415fc…` (v18 strata label, 792 lines, byte-identical since T3). `FU214` is the authoritative phase-19 census audit; its envelope puts the cohort's honest ceiling at **54/80 = 67%** (55/80 with the contingent phase `0x10`).

**Architecture (from the completed wave):** the cohort's bodies are already ported but **dead-ended** — `handlers.c` carries `{0x1D, NULL}` (phase `0x1D`) and `{0x20, NULL}` (action row `0x20`, body `0x84EEC` unported); `match_run_situation_side` (`match_run.c:2807`) already routes situation-8 → act `7`, but the selector never selects `8` because its inputs are inert. The producer chain is native and reachable only at a real **period completion** (FU-213): `FUN_0008AF38` (the clock) → period complete (`sec == limit`/`limit+aux`) → score tie-break (`[0x157AC5]` vs `[0x157AC7]`) → set `[0x157AC0]` → chooser `FUN_0008B9CC` reads the flag → installs act `0xC` (phase `0x22`) or `0xB` → `[0x157AC2]++` with the period-4 skip (`0x8B608`). Capture order is **top-down** (`T1 → T2 → T3 → T4`), because each consumer is inert until its producer lands. Consumers: `src/fifa96_engine/fifa96_match_run.c` (the clock/chooser/period seam + the frame body), `fifa96_match_state.c` (the period/limit state), `fifa96_match_handlers.c` (the phase/action tables + the row-`0x20` body), `fifa96_match_act.{c,h}` (the act seam). Serialized implementers; parallel read-only recon was the only parallel surface (already spent).

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan (engine + tests), Ghidra MCP read-only (`/FIFA96.EXE`; the authoritative recon tool — rea MCP only as a secondary read-only view, context7 only if SDL3/CMake docs are needed), SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU207…FU214`; audit `docs/ghidra/FU214_census_audit_phase19.md`; predecessor `docs/superpowers/plans/2026-10-12-fifa96-m2-phase18-producer-spine.md` + its final review; frontier `docs/ghidra/FU206_census_audit_phase18.md` (§4 the ranked cohort).

## Global Constraints

- Baseline: `make check` **110/110** (sanitized); wired **51/80**; README title **63%**; HEAD `30e57d8`. M1 `09b726b7…` (691 lines); M2 `441415fc…` (v18 label, 792 lines).
- Census arithmetic (this phase): action non-NULL `32` + phase `19` = `51`; `+N` for the rows the tasks prove producer-live. The exact target is whatever the tasks prove — **no hand-set number**; each task recomputes `floor(100 × wired/80)` when its count moves.
- **Wired census rule (AGENTS.md):** a row counts only when a ported path **installs + dispatches** it. Registering a table slot whose producer never fires does **not** count. Keep README title + the `docs/ENGINE.md` census line (title + dispatch sum-to-80) + `docs/PROGRESS.md` consistent in the same commit.
- Evidence floor: first-hand bytes/addresses; **fresh `get_xrefs_to` for census claims**; `/FIFA96.EXE` (case-sensitive); +0x100000 rule; **alias traps** (`dword[addr]>>16` = word at `addr+2`; `>>24` = byte at `addr+3`); verify CALL bytes before trusting a call target.
- **Determinism — the spine:** null backend source of truth; **M1 never moves**; M2 starts at the v18 bytes `441415fc…`. **Tape posture is case-by-case:** each task classifies its producer's tape-reachability first-hand and records the verdict; a landing that provably moves a presented frame *and* is native-faithful re-pins M2 with the written reason + frame diff; otherwise the producer lands gated/dormant and M2 stays byte-identical (the wave's verdict: **none of this cohort moves M2**). **Any unexplained presented-frame move is a regression to investigate, not a re-pin.**
- No fakes; unreachable → numbered legs (the wave already logged FU207-L1, FU208-L1/L8, FU209-L1, FU213-L1/L2, FU214-L1/L2); ISO-gated tests skip without the ISO.
- Commits `feat/fix/docs/test`; implementers commit and push per dispatch; controller verifies (`make check` + tape `cmp` + tree clean).

---

## Wave R — completed (R55–R62 → FU-207…FU-214, committed `30e57d8`)

Eight parallel read-only agents. Verdicts (each is a numbered leg where the producer is unreachable):

- **R55 → FU-207** — `[0x157AC0]` (OL-85) is a **boolean byte**, 7 writers (reset; clock `FUN_0008AF38` period-completion set-sites `0x8B2AC`/`0x8B2C1`/`0x8B3A2`/`0x8B5CD`; front-end `0x389A4`; act-7 terminal `0x89E8C`); **no engine cell; no live producer** (leg).
- **R56 → FU-208** — *premise correction:* situation `8` is gated on **`[0x157AC2]==4`** (`0x888C8`) **or bit-8** (`0x100`) of `[0x14C1D4]|[0x14C1D6]` (`0x4B0CB TEST AH,1`), **not** OL-85. Engine **already routes** sit-8 → act `7` (`action_handlers.c:1142`, `match_run.c:2807`) but dead-ends at `handlers.c:5117` `{0x1D, NULL}`. Legs: bit-8 vs bit-9 writer mismatch (FU208-L1).
- **R57 → FU-209** — act `7`/phase `0x1D` (`0x89868`) fully derived: 10-arm cascade; installer `0x899A2` = `FUN_0007D9A4(rec=[0x1587D4], code=0x20, stage=0, invoke=0)` (**deferred**); phase-`0x10` writer `0x898DD`. Not live-reachable.
- **R58 → FU-210** — action row `0x20` body `0x84EEC`: 7-arm CS table `0x84ED0`; render writes pos_x/y/z (`0x79B6C`) + anim_id/frame (`0x8CFAC`/`0x6E598`); wiring alone is **M2 byte-identical**.
- **R59 → FU-211** — phase `0x22`/act `0xC` (`0x8B900`): 3-stage act-slot timer; **does not** set `[0x157AC0]`; body already ported at `match_run.c:6158`; registration alone byte-identical + unreachable.
- **R60 → FU-212** — phase `0x1E`/act 8 (`0x8A798`): 3-stage 22-record restart sweep; **erratum** — not a code-`0xC`→`0x19` rewrite (`0x4C374` ignores EAX); de-compression answer = **M2 does not move, no re-pin**.
- **R61 → FU-213** — the clock/period crib: `FUN_0008AF38` (1/60 sub-sec, per-second loop, period-index dispatch `0x8AF28`); completion `sec == limit [+aux]` (limits `[0x15881A]`=60s `/ [0x15881C]`=30s); chooser `FUN_0008B9CC`; roll `0x8B58A` + period-4 skip `0x8B608`; end-of-match `FUN_0008BAF0` (`[0x158822]`). All inputs inert in the port.
- **R62 → FU-214** — census re-verified first-hand: **51/80 = 63%** (no correction); cohort deltas phase `0x1D` +1, row `0x20` +1, phase `0x22` +1 (phase `0x1E` +0, sit-8 producer +0); honest ceiling **54/80 = 67%**; **no candidate moves M2** ⇒ no re-pin forced.

---

### Task 1: T1 — the period-index roll + the extra-time decision (the live producer) (from R61/FU-213)

**Files:** `src/fifa96_engine/fifa96_match_run.c` (the period/clock seam + the frame body), `src/fifa96_engine/fifa96_match_state.c` (the period/limit state), `include/fifa96_engine/fifa96_match_run.h` (if a field/flag is added); tests + FU-213 status + `docs/ENGINE.md` + README title.
**Produces:** the live per-frame clock's period completion (FU-213: `sec == limit`/`limit+aux`) drives the extra-time decision — the score tie-break sets the engine's OL-85 flag, the chooser (`fifa96_action_phase_period_end`) receives the **live** `extra_time` (replace the hardwired `0u` at `match_run.c:2583`), and the period-index roll (`global_157ac2++` + the period-4 skip) runs. The flag/roll state is exposed for T2's situation-8 gate.
**Gate T1:** the clock completion + the tie-break + the roll covered by RED/GREEN + mutation-RED (a silent re-gate / wrong tie-break aborts the tape); **tape decision:** replay M2 — expected byte-identical (the tape reaches no period end; record the verdict); M1 never moves. Census recompute in the same commit (this task wires no row; it supplies the producer).
- [ ] RED test → implement → GREEN → gate (`make check` + tape `cmp`) → census/README update → commit `feat(engine,fu213): the period-index roll + extra-time decision`.

### Task 2: T2 — the situation-8 selection + the phase-0x1D slot (from R56/R57/FU-208/FU-209)

**Files:** `src/fifa96_engine/fifa96_match_run.c` (the situation selector + `match_run_situation_side`), `src/fifa96_engine/fifa96_match_handlers.c` + `include/fifa96_engine/fifa96_match_handlers.h` (the phase table slot `{0x1D, NULL}` → `fifa96_match_action_phase_1D`); tests + FU-208/FU-209 status + `docs/ENGINE.md` + README title.
**Produces:** the situation-8 gate (`global_157ac2 == 4` — or the bit-8 path, leg'd if its writer is absent, FU208-L1) selects situation `8`, which dispatches act `7` → phase `0x1D`, and the phase-`0x1D` slot is registered so the ported body `fifa96_match_action_phase_1D` (`match_run.c:5998`) actually runs. Phase `0x1D` is **counted iff** the fixture proves it dispatched on a live path; otherwise it stays a leg (no registration-only count).
**Gate T2:** the selector + the dispatch RED/GREEN + mutation-RED; the phase-`0x1D` body exercised end-to-end via a fixture that drives a live situation-8; tape posture case-by-case (per R56). Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p19-t2-*` → census/README update → commit `feat(engine,fu208): situation-8 selection + phase 0x1D slot`.

### Task 3: T3 — the row-0x20 body + the phase-0x1D arm-0 installer (from R57/R58/FU-209/FU-210)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` + `.h` (the row-`0x20` body `fifa96_match_action_20` + the action-table slot), `src/fifa96_engine/fifa96_match_run.c` (the phase-`0x1D` arm-0 coupled install `fifa96_match_run_action_bind(…, 0x20, 0, …)` mirroring `0x899A2`); tests + FU-209/FU-210 status + `docs/ENGINE.md` + README title.
**Produces:** the action row `0x20` body (`0x84EEC`: arms 0–6, the pos/anim writes) is ported and installed by the phase-`0x1D` arm-0 installer (`0x899A2`), and the action-table slot is registered. Row `0x20` is **counted iff** the fixture proves it dispatched; wiring alone is M2 byte-identical (R58), so the tape verdict is recorded but no re-pin.
**Gate T3:** the body + the installer RED/GREEN + mutation-RED; the pos/anim writes pinned; tape posture case-by-case (per R58 — dormant landing ⇒ byte-identical). Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p19-t3-*` → census/README update → commit `feat(engine,fu210): action row 0x20 body + phase 0x1D installer`.

### Task 4: T4 — phase 0x22 / act-0xC registration + the end-of-match (from R59/R61/FU-211/FU-213)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (the phase-table slot `0x22` → `fifa96_match_action_phase_22`, already ported at `match_run.c:6158`), `src/fifa96_engine/fifa96_match_run.c` + `fifa96_match_state.c` (the end-of-match `FUN_0008BAF0` / `[0x158822]` if in scope — **the live flag feeding `fifa96_action_phase_period_end` is T1's; do not re-touch that call site**); tests + FU-211/FU-213 status + `docs/ENGINE.md` + README title.
**Produces:** phase `0x22` is reached when the live extra-time decision installs act `0xC` (its body is render-invisible per R59), and the end-of-match word is modelled. Phase `0x22` is **counted iff** the fixture proves it dispatched; otherwise it stays a leg.
**Gate T4:** the registration + the chooser→act-`0xC` route RED/GREEN + mutation-RED; tape posture case-by-case (per R59 — neutral even if fired). Census recompute in the same commit.
- [ ] RED test → implement → GREEN → gate + smoke `p19-t4-*` → census/README update → commit `feat(engine,fu211): phase 0x22 registration + end-of-match`.

### Task 5: T5 — the phase-0x1E honest bound + the residual legs (from R60/R62/FU-212/FU-214)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (the phase-`0x1E` slot decision), tests + FU-212/FU-214 status + `docs/ENGINE.md` (legs) + `docs/PROGRESS.md`.
**Produces:** phase `0x1E`/act 8 keeps its compressed model (R60: the faithful 22-record sweep is a timing-only delta, no M2 move — **not** wired this phase, logged as the de-compression leg with owner FU212); every cohort row the tasks could not prove live is recorded as an honest numbered leg with its native owner (FU207-L1, FU208-L1/L8, FU209-L1, FU211-L1, FU213-L1/L2, FU214-L1/L2). No fake producer.
**Gate T5:** the compressed-model pin + the leg register; tape byte-identical. Census recompute only if a count moved.
- [ ] RED/pin test → implement → GREEN → gate → census/PROGRESS update → commit `docs(engine,fu212): phase-0x1E honest bound + residual legs`.

### Task 6: T6 — acceptance v19 + riders + smoke + docs + final review

**Files:** `tests/test_engine_m2.c` (+ golden iff genuinely moved — the wave says it will not), `docs/ENGINE.md`, `docs/PROGRESS.md`, `docs/screens/*` (if the SDL3 smoke frames move).
**Produces:** the v19 strata (a re-pin iff earned with the written reason + frame diff, else the strata-only label — the phase-15 v14→v15 precedent); the producer riders (mutation-RED: a silent producer re-gate / wrong tie-break / selector slip aborts the tape); the honest smoke record; the remaining legs with owners; the census recomputed against R62's audit.
- [ ] Tests + riders → tape decision (byte-identical or an earned re-pin; record it) → smoke → docs → commit `test(engine): phase-19 acceptance tape v19 and docs` → whole-plan final review (one fix wave max) → close.

---

## Self-Review Notes

- **Coverage:** the period/clock producer + the extra-time decision → R61/T1; the situation-8 selector + the phase-`0x1D` slot → R56/R57/T2; the row-`0x20` body + the installer → R57/R58/T3; phase `0x22` + the end-of-match → R59/R61/T4; the phase-`0x1E` bound + the residual legs → R60/R62/T5; census/acceptance → T6.
- **Dependency order:** Wave R (done) → T1 → T2 → T3 → T4 → T5 → T6. **T1 first** (the live producer everything is inert without); T2 next (the selector + the phase-`0x1D` body, which T1's roll feeds); T3 (the coupled row-`0x20` body the phase-`0x1D` arm installs); T4 (phase `0x22`, independent of T2/T3 but serialized); T5 (the residual bound); T6 (acceptance).
- **Split rule:** if the T1 producer probe exceeds ~4 tasks, split it into its own plan (M2 spec §12 rule). The honest outcome may be **+0** rows (all bodies wired-but-dormant, all producers leg'd) — that is a legitimate close (the phase records the legs and does **not** fake a count); no row is counted off registration alone.
- **Risk register:** (a) the period completion may not be reachable in the ported flow (the tape reaches no period end) — then T1's producer is fixtured + leg'd, not faked; (b) the bit-8 writer gap (FU208-L1) may block the non-`global_157ac2` sit-8 path — leg'd; (c) M2 may move unexpectedly — that is a **regression** to investigate, never a silent re-pin.

## Landing notes (post-execution)

_(filled by the controller as tasks land)_
