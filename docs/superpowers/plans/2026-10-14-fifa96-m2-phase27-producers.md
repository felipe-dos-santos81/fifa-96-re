# FIFA 96 M2 phase-27 Stage-2 — the keeper-CPU installer (producers)

> **For agentic workers:** REQUIRED SUB-SKILL: `superpowers:subagent-driven-development`. Steps use checkbox (`- [ ]`).

**Goal:** Wire the ported keeper-CPU machines into a **live frame** (the FU296 Seam A), land the bounded gate/actor producers, and either raise the ladder install live (the camera unlock) or close honestly as a Layer-1 reach + a Layer-2 waiver — with every unported surface a numbered leg.

**Governing spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase27-keeper-installer-design.md` §3/§5/§6.
**Frozen authority:** the phase-27 wave `docs/ghidra/FU290..FU297` (reviewed + fixed, `7d013e0`) + `FU273/FU282/FU283/FU285`.

**The wave's binding findings (reviewed):**
- **FU296 Seam A = GO** — call `fifa96_keeper_cpu_step` in the `i==0` branch of `fifa96_match_entities_team_update` (`src/fifa96_match_entities.c:704`), before the single `action()` dispatch (`:708`); the frame-level call double-dispatches. Link `fifa96_keeper_cpu.c` into the engine lib (`CMakeLists.txt:432-450`).
- **The blocker: P13 — the stage index `CX` has NO live producer** (`stage==0` → the ladder returns at `0x781E1`, installing nothing). FU290 derived the full `CX ∈ {0,1,4,5,A}` machine but left four input families unported: the `0x10E1xx` stage tables, the `[0x157C4C/48/44/5C]` latch cells, the gate words, and the per-record `[[rec+4]+0xB/+0x10]` sub-fields.
- **Bounded wins:** FU291 — the gate value is computable live (`prepass_ah2 = (input_range[(side^side_swap)&1]>>9)&1`), zero-δ; FU294 — the pan/camera setter is **already ported**; FU295 — the keeper-route actor producers `0x76DBF`/`0x77568` are **already LIVE**.
- **Row `0x1A` stays NULL** (FU297/FU284). A flip would quote `60/80 = 75%` but be a dead count.

## Global Constraints

- `make check` green (strict `-Wall -Wextra -Werror`, ASan/UBSan; ~143 s → ≥600 s timeout); work on `main`; commit per task; push per task.
- **M1 `09b726b7…` immutable.** **M2 `b281a9b2…` (v22):** re-pin only if a **live** path changes the tape — written reason + frame diff. Expect byte-identical through the staged tasks.
- **The live-chain rule:** a fixture that stages a gate is a fake; staging a producer's **input** is legitimate; an unraised surface is a numbered **LIVE-LEG**. Never fabricate a stage index.
- **Census `59/80`** — moves only on a proven live install+dispatch (the four-doc same-commit rule; `floor(100×wired/80)`).
- Ghidra read-only; no fakes.

## File Structure

| Path | Responsibility |
|---|---|
| `src/fifa96_engine/fifa96_match_entities.c` | the FU296 seam (the CPU step call in `team_update`) |
| `src/fifa96_loader/fifa96_keeper_cpu.c` / `include/fifa96_loader/fifa96_keeper_cpu.h` | the record-0 machine + ladder; the gate value; the stage-select inputs |
| `CMakeLists.txt` | link `fifa96_keeper_cpu` into the engine lib |
| `src/fifa96_engine/fifa96_match_run.c`, `fifa96_match_handlers.c` | the stage-select cell producers (FU292); the actor-route |
| `tests/test_keeper_cpu.c`, `tests/test_engine_match_frame.c` | the staged install reach + mutation-RED riders |
| `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md:90`, `tests/test_engine_m2.c` | the census + leg register (close-out) |

---

### Task 1: The gate + the RED live-reach test

**Files:** Test `tests/test_engine_match_frame.c`

- [ ] **Step 1:** `make check` green; census pins `action_wired == 36u && phase_wired == 23u`; M1/M2 `cmp` ×3; record the baseline `camera.pos_y == 0`.
- [ ] **Step 2: RED test (disabled)** — drive one `fifa96_match_run_frame` with the CPU step wired (T3) but the stage index **staged** as a producer input: assert the ladder installs row `0x1B` (`records[0].code == 0x1B`), the emit reaches `phase == 9`, and `camera.pos_y != 0`. Commit DISABLED (skip-guarded, the phase-26c precedent — the plan's T1 ruling carries): `test(fu296): phase-27 — the installer reach test (RED)`.

### Task 2: The FU291 gate value (bounded win)

**Files:** `fifa96_keeper_cpu.c/.h`; Test `tests/test_keeper_cpu.c`

- [ ] **Step 1: Failing test** — assert `prepass_ah2 == (input_range[(side ^ side_swap) & 1] >> 9) & 1` from live state (replacing the caller-supplied field).
- [ ] **Step 2: Implement** the fold (`fifa96_input_range_words` → `mr->input_range`; the bit-9 gate `input.c:97-98`).
- [ ] **Step 3:** PASS; `make check`; M2 `cmp` (zero-δ expected).
- [ ] **Step 4: Commit** `feat(fu291): compute the prepass gate value from input_range`.

### Task 3: The FU296 Seam A (the engine wiring)

**Files:** `src/fifa96_engine/fifa96_match_entities.c`; `CMakeLists.txt`; Test `tests/test_keeper_cpu.c`

- [ ] **Step 1: Failing test** — assert `fifa96_keeper_cpu_step` runs exactly once per team per frame with **no** double-dispatch of record actions (the record-0 machine's install must feed the existing single `action()`/`match_run_dispatch_entity` path, not add one).
- [ ] **Step 2: Implement** — call the step in the `i==0` branch (`:704`) before `action()` (`:708`); bind `interlock_157c5d`/`latch_157c5c`/`force_install` ↔ the run cells; link the module into the engine lib.
- [ ] **Step 3:** PASS; `make check`; M2 `cmp` (expect byte-identical — the ladder stays unraised with `stage==0`).
- [ ] **Step 4: Commit** `feat(fu296): wire the keeper-CPU step into the per-frame team update (Seam A)`.

### Task 4: The staged Layer-1 installer reach

**Files:** Test `tests/test_engine_match_frame.c`

- [ ] **Step 1:** Enable the T1 test: with the stage index staged as a producer **input**, the machine raises the ladder, the ladder installs row `0x1B` (case 4) invoke-now, the row's wide/self-actor path reaches the emit (`phase == 9`), row `0x1E` dispatches, the claim reaches stage 5, the release fires → `camera.pos_y == 0x50` (+ the row-0E window). GREEN.
- [ ] **Step 2: Mutation-RED riders** — stub (a) Seam A, (b) the ladder install, (c) the claim bridge — each must break it.
- [ ] **Step 3:** `make check`; M2 `cmp` (staged producers are test-only → expect byte-identical); commit `test(fu296): the staged installer reach — green (Layer 1)`.

### Task 5: The stage-select input families (the live trigger — the hard task)

**Files:** `fifa96_keeper_cpu.c`; `src/fifa96_engine/fifa96_match_run.c`/`fifa96_match_handlers.c`; Test `tests/test_keeper_cpu.c`

**Interfaces:** FU290 legs L1–L5 + FU292 (the latch/geometry cells: `0x157C4C`/`0x157C48`/`0x157C44`/`0x157C2C`/`0x157C5C`, `0x1577F4/F8`, `0x1577BC`, `0x157821`, `0x158740`).

- [ ] **Step 1: Failing test** — assert the `CX` machine yields a non-zero stage from real engine state for a keeper scenario (or record the honest leg).
- [ ] **Step 2: Implement** each portable input: the `0x10E1xx` stage tables, the latch/geometry cells (FU292's writer set), the per-record sub-fields — feeding FU290's derived `CX` machine. **Keep every genuinely-unported input a numbered leg** — never a fabricated stage.
- [ ] **Step 3:** PASS (or the legs recorded); `make check`; M2 `cmp`; **if a live install now fires**, the census may move (the four-doc same-commit rule) and M2 may re-pin (written reason + diff).
- [ ] **Step 4: Commit** `feat(fu290): the stage-select inputs — the ladder's live trigger` (or `docs(fu290): rule the stage-select inputs legs`).

### Task 6: The FU295 actor-route + the FU294 pan residuals

**Files:** `fifa96_match_handlers.c`, `fifa96_match_run.c`; Test `tests/test_engine_match_frame.c`

- [ ] **Step 1: Failing test** — the actor lift `FUN_0008C33C` (`0x8C3BE`) + `0x76AD8` (the row-`0x1A` body actor set — hmm, row `0x1A` stays NULL; rule accordingly).
- [ ] **Step 2: Implement** the portable pieces (the `0x8C3BE` actor lift); keep the rest legs.
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`.
- [ ] **Step 4: Commit** `feat(fu295): the controlled-side actor lift`.

### Task 7: The Layer-2 acceptance

**Files:** `tools/fifa96_play.c` (if needed)

- [ ] **Step 1:** `make game` (`DISPLAY=:1`): does the follow/pan camera move live? If yes, record the Layer-2 pass; if no (P13 unraised), record the **honest waiver** (spec §6) with the Layer-1 proof as the landing.
- [ ] **Step 4: Commit** `test(fu296): the Layer-2 camera acceptance (or the honest waiver)`.

### Task 8: The close-out

**Files:** `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md:90`, `tests/test_engine_m2.c`; `tests/golden/engine/m2-frames.txt` (only if re-pinning)

- [ ] **Step 1: Census** — if Task 5 raised a live install+dispatch, recompute + update the four docs + pins in one commit; else hold `59/80`.
- [ ] **Step 2: M2** — re-pin (v23) with a written reason + diff if a live path changed; else byte-identical.
- [ ] **Step 3: Leg register** — append as quoted errata: FU290 (`CX ∈ {0,1,4,5,A}`; FU283 §3 errata); FU291 (the gate value ported); FU294 (the camera setter already ported; FU285 §2 errata); FU295 (the cell-split correction; `0x76AD8`); FU296 (Seam A; P13 the blocker); FU297 (row `0x1A` NULL; the `60/80` dead-count warning; the +116 drift); the FU292 cells; the FU293 L1 interpolation legs.
- [ ] **Step 4: `docs/PROGRESS.md`** close-out (the Layer-1 reach + the Layer-2 verdict).
- [ ] **Step 5:** `make check`; M1 byte-identical. Commit + push `docs(fu296): phase-27 close-out — the census + the leg register`.

---

## Self-Review

- **Spec coverage:** §3's 8 surfaces map to T2 (FU291), T3 (FU296), T5 (FU290/FU292), T6 (FU295/FU294); §5 acceptance = T1/T4/T7; §6 honest-stop is the Global Constraints + T5/T7.
- **The honest risk:** P13 (the stage index) is the phase's crux — if T5 cannot raise it live from ported state, the phase closes as a Layer-1 reach + a Layer-2 waiver (as 26a/26c), with the four input families named as legs. The census holds.
- **Placeholders:** none.

## Execution Handoff

**1. Subagent-Driven (recommended).** **2. Inline.**
