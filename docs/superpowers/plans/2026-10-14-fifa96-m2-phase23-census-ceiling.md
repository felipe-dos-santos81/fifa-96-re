# FIFA 96 M2 Phase-23 — The Census Ceiling Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Raise the honest action-dispatch census from **55/80 = 68%** to **58/80 = 72%** by wiring the three proven-attainable rows — phase `0x1E`, action `25`, action `15` — correcting the stale census evidence, and probing the gated `0x13`/`0x14` upside.

**Architecture:** The phase-23 recon wave (`FU240…FU247`) froze the frontier. Three movers survive: (T2) action `25` — the installer is already live in `fifa96_match_phase_machine_step`; only the body `0x880CC` is unported; (T3) action `15` — the installer is the phase-5 arm `0x8D433`, the body `0x87CD0` clean; (T4) phase `0x1E` — register the ported sweep and re-route the compressed `bx!=0` path. (T5) corrects the stale census evidence and close-out; (T6) probes the `0x13`/`0x14` RNG gate (gated upside, +2).

**Tech Stack:** C (C99), `fifa96_engine` match handlers + the `fifa96_loader` action dispatch; the null backend as determinism source of truth; CTest under ASan/UBSan `-fno-sanitize-recover=all`.

**Spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase23-census-ceiling-design.md` (argue from it; it travels with this plan).

**Design authority (frozen recon — read before touching code):**
- `docs/ghidra/FU243_no_installer_cluster.md` — **rows `25`/`15` (T2/T3 authority)**: the ECX-path installer refutation.
- `docs/ghidra/FU241_phase_1e_decompression.md` — **phase `0x1E` (T4 authority)**: the all-terms lift + the exact pin flips.
- `docs/ghidra/FU242_installer_census.md` (the exhaustive census + `FUN_0008CEB8` ECX path), `FU240_phase_1314_producer.md` (T6: the `0x13`/`0x14` RNG gate), `FU244…FU247` (the ceiling legs).

## Global Constraints

- Every address/byte claim cites a disassembly or a reconciled raw-byte read; unprovables become numbered legs, never asserted. (AGENTS.md evidence rules.)
- The M1 golden `tests/golden/engine/m1-frames.txt` (`09b726b7…`, 691 lines) is **immutable** — stop and report if it moves.
- The M2 golden `tests/golden/engine/m2-frames.txt` (`b281a9b2…`, v22, 6522 lines) **must stay byte-identical** in this phase: all three movers are tape-dormant or render-invisible (FU241 §6.2, FU242). Any drift is a regression to investigate, never a re-pin.
- Determinism source of truth is the null backend (`src/fifa96_engine/platform_null.c`).
- **No faked producers.** A row counts only when a ported path installs AND dispatches (AGENTS.md). Wire the slot only after its live dispatch is proven by a test.
- Build/test gate: `make check` (strict `-Wall -Wextra -Werror` + full CTest). Single: `ctest --test-dir build -R <name> --output-on-failure`; binaries run from the repo root.
- Census is **derived**: after T2/T3/T4 the count is `35 action + 23 phase = 58/80`; recompute and update the README title + `docs/ENGINE.md` + `docs/PROGRESS.md` + `AGENTS.md` **in the same commit** as the moving task.
- Commit types `feat|fix|docs(fuNNN)|test|chore`; work on `main`; `git push origin main` per task.

## File Structure

- `src/fifa96_engine/fifa96_match_handlers.c` — the action table (`:5097..5186`) + phase table (`:5188..5259`): register `25`/`15`/`0x1E`, port the bodies, correct the stale evidence strings.
- `include/fifa96_engine/fifa96_match_handlers.h` — the new body prototypes (`fifa96_match_action_25`, `fifa96_match_action_15`) if exported.
- `src/fifa96_engine/fifa96_match_run.c` — the phase-`0x1E` de-compression re-route (T4).
- `tests/test_engine_match_act.c` — the per-row dispatch home (T2/T3/T4).
- `tests/test_engine_match_frame.c` — the phase-`0x1E` pin flip (T4).
- `tests/test_engine_m2.c` — the census pins (`:3666`, `:4041`) + the mutation-RED riders (T5).
- `docs/ghidra/FU2xx_*.md` — appended errata only, never rewritten (T5).

---

### Task 1: Determinism + feasibility gate (no re-pin)

**Authority:** spec §5 T1. Prove the baseline is byte-stable and the three movers are tape-neutral before any edit.

**Files:**
- Test: `tests/test_engine_m2.c` (run only; no edit).

- [ ] **Step 1: Recompute the census against the native tables.**

Run: `./build/test_engine_m2 2>&1 | tail -5`
Expected: `action_wired == 33u && phase_wired == 22u` (55/80 = 68%) — the baseline.

- [ ] **Step 2: Prove ≥3-replay byte-stability.**

Run: `for i in 1 2 3; do ./build/test_engine_m2 | sha256sum; done`
Expected: all three hashes equal `b281a9b2…`; M1 `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt` clean.

- [ ] **Step 3: Gate.** If any replay diverges → STOP and report (a fixture-only proof is the fallback). Otherwise proceed; no commit.

---

### Task 2: Action `25` — the body port + the live installer (census +1)

**Authority:** FU-243 §4.2/§6 (`0x880CC` body, slot `0x110774`; installer `0x8D79F MOV ECX,0x25 → 0x8D7AD CALL 0x8CEB8`) + FU-242 §2 (`FUN_0008CEB8`; the ECX code path) + FU-82 (the 7-arm table `0x880B0`). The installer is **already modelled** in `fifa96_match_phase_machine_step` (`code = ac5!=ac7 ? 0x25 : 3`) — only the body is unported.

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c` (port `fifa96_match_action_25` from the FU-243 §4.2 byte listing; register `:5170` `{0x25, NULL, …}` → `{0x25, fifa96_match_action_25, …}`; correct the `:5171` evidence string).
- Modify: `include/fifa96_engine/fifa96_match_handlers.h` (declare the body if not `static`).
- Test: `tests/test_engine_match_act.c`.

**Interfaces:**
- Consumes: the FU-138 entity pool helpers, the FU-82 `0x880B0` arm table.
- Produces: `int fifa96_match_action_25(fifa96_match_run *mr, fifa96_match_entity *e, unsigned stage)` returning `FIFA96_OK`.

- [ ] **Step 1: Write the failing dispatch test.**

In `tests/test_engine_match_act.c`, mirror the row-`20` dispatch test:

```c
/* FU-243: row 25 dispatches on a live phase-machine install (was wrongly
 * believed installer-absent, FU189-L2 refuted). */
static void test_action_25_dispatches(void) {
  struct fixture f; make_fixture(&f);
  fifa96_match_entity *e = fifa96_match_entities_get(&f.mr, 0);
  int rc = fifa96_match_dispatch_action(&f.mr, 0x25u, 0u);
  assert(rc == FIFA96_OK);                 /* registered + dispatched, not UNSUPPORTED */
  drop_fixture(&f);
}
```
Call it from `main`. (`e` unused is fine only if the signature needs it — otherwise drop the line.)

- [ ] **Step 2: Run it — expect failure.**

Run: `ctest --test-dir build -R test_engine_match_act --output-on-failure`
Expected: FAIL — `FIFA96_ERR_UNSUPPORTED` (slot NULL).

- [ ] **Step 3: Port the body** `fifa96_match_action_25` from the FU-243 §4.2 byte listing of `0x880CC` (the 7-arm table `0x880B0`, the stage walk, the `0x8DCD4` metric + the FU-82 sinks). Follow the FU-82 port contract; unported sinks become numbered legs.

- [ ] **Step 4: Register the slot + correct the evidence.**

In `fifa96_match_handlers.c`, the action table entry:

```c
    {0x25, fifa96_match_action_25,
     "FU-243 §4.2/§6 (phase-23 T2): row 25 body 0x880CC ported (FU-82 7-arm table 0x880B0). "
     "COUNTED: the installer 0x8D79F/0x8D7AD (MOV ECX,0x25 -> FUN_0008CEB8 -> FUN_0007D9A4) is on the "
     "live FUN_0008D098 path, already modelled by fifa96_match_phase_machine_step (code = ac5!=ac7 ? 0x25 : 3). "
     "The prior 'row 25 has NO static installer' (FU189-L2) is REFUTED (the EDX/immediate-only scan missed the "
     "ECX-carried code, FU-242 §2). Sinks/legs per FU-243 §5; OL-9"},
```

- [ ] **Step 5: Run the tests — expect pass; bump the pin.**

Run: `ctest --test-dir build -R 'test_engine_match_act|test_engine_m2' --output-on-failure`
Then update `tests/test_engine_m2.c:3666` and `:4041` `action_wired == 33u` → `34u`. Re-run; expected PASS, M2 `b281a9b2…` unmoved (row 25 is tape-dormant).

- [ ] **Step 6: Commit.**

```bash
git add src/fifa96_engine/fifa96_match_handlers.c include/fifa96_engine/fifa96_match_handlers.h tests/test_engine_match_act.c tests/test_engine_m2.c
git commit -m "feat(engine): wire action row 25 (the ECX-path installer refutation, FU-243)"
```

---

### Task 3: Action `15` — the body + the phase-5 install arm (census +1)

**Authority:** FU-243 §4.3/§6 (`0x87CD0` body, slot `0x110734`; installer = the phase-5 arm `0x8D433`/`0x8D44B MOV ECX,0x15 → 0x8D45C CALL 0x8CEB8`, the row-14 celebration sibling, FU-184-L8) + FU-243-L2 (the arm is **not yet ported**).

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c` (port `fifa96_match_action_15` from the FU-243 §4.3 byte listing; register `:5139` `{0x15, NULL, …}` → `{0x15, fifa96_match_action_15, …}`; correct the `:5139` "stub bucket" string).
- Modify: `src/fifa96_engine/fifa96_match_run.c` (port the phase-5 install arm `0x8D433` that installs code `0x15` — FU-243-L2).
- Test: `tests/test_engine_match_act.c`.

**Interfaces:**
- Consumes: the FU-184 celebration-scatter cluster (row-14's sibling machinery).
- Produces: `int fifa96_match_action_15(fifa96_match_run *mr, fifa96_match_entity *e, unsigned stage)`.

- [ ] **Step 1: Write the failing test** (mirror Step 2.1 naming it `test_action_15_dispatches`, asserting `FIFA96_OK`). Run it — expect FAIL (`UNSUPPORTED`).

- [ ] **Step 2: Port the body** `fifa96_match_action_15` from FU-243 §4.3's `0x87CD0` listing (it decodes cleanly; the state-5 arm per FU-184-L8).

- [ ] **Step 3: Port the phase-5 install arm** `0x8D433` (FU-243-L2): the side-compare `0x8D441 CMP AL,[EDX+0x826]; JNZ` then `MOV ECX,0x15`. If this arm cannot be ported faithfully (FU-243-L2's caveat), **do not fake it** — leave row 15 a leg, revert the slot, and record the ruling in the ledger (the census target drops to +2 → 57/80 = 71%).

- [ ] **Step 4: Register + correct the evidence string**, then run `ctest -R 'test_engine_match_act|test_engine_m2'`; bump `action_wired == 34u` → `35u` at both pins; expect M2 `b281a9b2…` unmoved (tape-dormant).

- [ ] **Step 5: Commit** (`feat(engine): wire action row 15 (the phase-5 celebration sibling, FU-243)`).

---

### Task 4: Phase `0x1E` — the de-compression (census +1)

**Authority:** FU-241 §6.3 (the exact lift terms) + §6.1/§6.2 (render-invisible, golden never enters `bx != 0`). **No golden re-pin.**

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c` (`:5249` `{0x1E, NULL, …}` → `{0x1E, fifa96_match_action_phase_1E, …}`; the evidence string).
- Modify: `src/fifa96_engine/fifa96_match_run.c` (re-route the compressed `bx != 0` path to `fifa96_match_act_dispatch(8, 0, 1)`).
- Test: `tests/test_engine_match_frame.c` (the pin flip), `tests/test_engine_match_act.c`, `tests/test_engine_m2.c` (the census rider).

**Interfaces:**
- Consumes: `fifa96_match_action_phase_1E` (already ported), `fifa96_match_run_set_piece`.
- Produces: the phase-`0x1E` row registered + dispatched via act 8.

- [ ] **Step 1: Flip the pins to the failing state.** In `tests/test_engine_match_frame.c::test_phase_1E_compressed_model_pin` and the `fifa96_match_dispatch_phase(&mr, 0x1E) == FIFA96_ERR_UNSUPPORTED` assertion, invert to expect the row to be registered/dispatched (FU-241 §6.3 items 3-4).

- [ ] **Step 2: Run — expect failure** (slot still NULL).

- [ ] **Step 3: Register the slot + re-route.** `handlers.c:5249` → `{0x1E, fifa96_match_action_phase_1E, …}`; `match_run.c` the `bx != 0` branch → `fifa96_match_act_dispatch(mr, 8u, 0u, 1u)` (FU-241 §6.3 item 2).

- [ ] **Step 4: Run + bump the pins.** `ctest -R 'test_engine_match_frame|test_engine_match_act|test_engine_m2'`; `phase_wired == 22u` → `23u` at `:3666`/`:4041`; the `:3701` `phase_table[0x1E].fn == NULL` pin flips. Expect M2 `b281a9b2…` unmoved (FU-241 §6.2).

- [ ] **Step 5: Commit** (`feat(engine): register phase 0x1E (the restart-sweep de-compression, FU-241)`).

---

### Task 5: The census close-out + the stale-evidence sweep

**Authority:** spec §5 T-closeout; FU-243 §1 (the refuted strings), FU-247 §3 (the stale tape counts + the FU185-L6 erratum), FU-241 §7.

**Files:**
- Modify: `README.md` (title `68%` → `72%`), `docs/ENGINE.md` (the status line + the census bullets), `docs/PROGRESS.md` (a phase-23 entry), `AGENTS.md:90` (`55/80 → 68%` → `58/80 → 72%`).
- Modify: `docs/ghidra/FU-189*` / `FU-185*` / `FU-192*` — **appended errata only** (never rewritten): the row-25 installer refutation, the `word[+0x6B]/[+0x6D]/[+0x7D]` angle-input erratum, and the fresh v22 tape counts (`0x01` 953, `0x13` 2640, `0x14` 20).

- [ ] **Step 1: Add the mutation-RED riders** to `tests/test_engine_m2.c` (`run_phase23_riders`): (a) disable the row-25 body → `dispatch_action(0x25)` aborts; (b) the row-15 body; (c) the phase-`0x1E` re-route → the `dispatch_phase(0x1E)` aborts. Each must be verified mutation-RED (guarded code disabled → the assert aborts).

- [ ] **Step 2: Recompute the census.** `floor(100 × 58/80) = floor(72.5) = 72`; update every surface in one commit; `make check` **green** (sanitized); M1/M2 unmoved.

- [ ] **Step 3: Commit** (`docs(engine): the phase-23 census close-out (58/80 = 72%) + the stale-evidence sweep`).

---

### Task 6: The `0x13`/`0x14` RNG gate — the gated upside probe

**Authority:** FU-240 §2/§4 (FU240-L2: dispatching `0x13`/`0x14` injects shared-`0x92AC8` RNG draws into `0x6E244`, which the live FU-239 ET chooser would reach on the v21 golden tail).

**Files:**
- Test: `tests/test_engine_match_completion.c` (a characterization probe; no wire).

- [ ] **Step 1: Characterize the injection.** Drive the ET chooser on a fixture and assert whether the `0x6E244` walk consumes the shared RNG stream on the frames the golden reaches (FU-240 §2). If it does → the rows stay **gated** (a leg); if it provably does not touch the tape → record it as a Stage-2 follow-up candidate (+2).

- [ ] **Step 2: Rule + commit** the probe result as a leg or a follow-up (`docs(fu240): the 0x13/0x14 RNG-gate probe result`). **Do not wire** without the probe clearing; the guaranteed floor already stands at 58/80.

---

## Self-Review

- **Spec coverage:** the A-movers (phase `0x13`/`0x14`, phase `0x1E`) → T6, T4; the newly-found rows `25`/`15` → T2/T3; the ceiling legs `FU244…FU247` → recorded (no task, they are legs); the census close-out → T5. The ruling's revised floor (58/80) is the plan's target.
- **Placeholders:** none — each task names its authority, files, interfaces, and TDD steps.
- **Type consistency:** `fifa96_match_action_25`/`_15`/`_phase_1E`, `fifa96_match_dispatch_action`, `fifa96_match_dispatch_phase`, the census pins (`:3666`/`:4041`) are consistent across tasks.
- **Caveat:** T3 (row 15) is the heaviest and may resolve to a leg (+2 instead of +3); T5 owns the resulting census recompute.

## Handoff

Hand this plan to `superpowers:subagent-driven-development`: one implementer per
task (T1 → T2 → T3 → T4 → T5 → T6), a task review after each, and the whole-branch
review at the end. `make check` green before any push.
