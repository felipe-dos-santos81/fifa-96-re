# FIFA 96 M2 phase-21 Stage-2 — the live period-end spine (producer plan)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Turn the derived period-end spine from *tape-dormant* into *live* — a running match completes period 1 (half-time → period 2) and a differing-score regulation end rolls into extra-time entry — and prove it by extending the committed M2 acceptance tape (a documented re-pin).

**Architecture:** The chain already exists (FU-143 `phase_drive` → FU-213 chooser `fifa96_action_phase_period_end` → `fifa96_match_run_period_setup(mode 0)`); the recon (FU-223…FU-230) proved the missing pieces are (a) the **live invocation** of the mode-0 producer on every frame (not only the index-4 gate), (b) the **`mark_over` re-point** so the first completion no longer exits the run, and (c) the **tape extension**. No new phase row, no new body — activation + a lifecycle trigger fix + the acceptance tape.

**Tech Stack:** C11 / CMake / CTest, ASan+UBSan (`-fno-sanitize-recover=all`), the null backend as the determinism source of truth, Ghidra MCP (read-only) for native facts.

**Spec:** `docs/superpowers/specs/2026-10-13-fifa96-m2-phase21-live-period-end-design.md` (§5 T1–T6 + §10 errata E1–E4). Frozen native evidence: `docs/ghidra/FU223…FU230`. Predecessor plan: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase21-recon-ahead.md`.

## Global Constraints

- **Baseline (HEAD `cb4cbf7`):** `make check` **110/110** (sanitized, strict); wired **55/80 = 68%** (`33 action + 22 phase`; dispatch `24 UNSUP / 55 OK / 1 NOTF`); M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (691 lines, **immutable**); M2 `441415fc423b05b2957f0a0bd51efa44f7f4651b4f8c07fcd6fb5ed6a8da9506` (792 lines, v20).
- **M1 never moves.** M2 re-pins **only** in T6, with a written reason + frame diff.
- **Determinism source of truth = the null backend** (`platform_null.c`); the SDL3 backend stays consistent with it.
- **No fakes.** Every unreachable native is a numbered leg with its native owner; the stand-in cells stage the native BSS default (`0`).
- **Evidence floor (AGENTS.md):** every native claim cites a fresh disassembly or the frozen `FU2xx` slice; addresses are `/FIFA96.EXE` flat addresses; LE globals at `+0x100000`.
- **Commits:** `feat` / `fix` / `docs(fuNNN)` / `test` / `chore`; work on `main`; push per dispatch. Update the README title + `docs/ENGINE.md` census line + `docs/PROGRESS.md` + `AGENTS.md` **in the same commit** only if the wired count moves (this phase expects none: 68%).
- **Test commands:** `make check` (gate); single: `ctest --test-dir build -R <name> --output-on-failure`; direct from repo root: `./build/test_engine_m2`.

## Errata the plan carries (spec §10)

- **E1** — the extra-time route needs a **differing** regulation-end score (`0x8B2AC`); a 0-0 tie leaves `[0x157AC0]` clear and never arms ET. The tape scripts a **narrow lead** at the regulation whistle.
- **E2** — `period_setup_live` is already `1` (`fifa96_match_run.c:2225`); **no flip**. T4's real work is the live mode-0 cadence.
- **E3** — the period length is already staged (`120`/`40`, byte-exact); T2 is a regression pin, not a producer.
- **E4** — the end machine (`FUN_0008BAF0`/`[0x158822]`) is **not** required; T5 re-points `mark_over` only (conditional on the ET flag being set at ET entry, FU228-L6).

## File structure

| Path | Responsibility | Task |
|---|---|---|
| `tests/test_engine_m2.c` | the M2 acceptance tape, the period-end feasibility probe, the phase-21 riders, the re-pin | T1, T6 |
| `tests/test_engine_match_completion.c` | the period-length pin, the half-time completion route, the `mark_over` trigger | T2, T5 |
| `tests/test_engine_match_frame.c` | the roll/flag staging pins, the chooser half-time select, the `mark_over` boundary | T3, T5 |
| `src/fifa96_engine/fifa96_match_run.c` | the live mode-0 cadence (T4), the `mark_over` re-point (T5) | T4, T5 |
| `tests/golden/engine/m2-frames.txt` | re-pinned at T6 (v20 → v21) | T6 |
| `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md` | census/re-pin record (only if the count moves; expected none) | T6 |

---

### Task 1: T1 — the determinism + feasibility probe

**Objective (spec §5 T1).** Extend the tape's *scripted run* (in a transcript-neutral probe) to drive a period completion with the spine's producers still gated; prove null-backend determinism across ≥3 replays; characterize exactly what the run does now (**no re-pin**). If not byte-stable → STOP and report.

**Files:**
- Modify: `tests/test_engine_m2.c` (add `run_period_end_probe` + a ≥3-replay determinism check; call it from `main`). Do **not** touch `M2_KEYS`, `M2_STAGE_ROWS`, or `tests/golden/engine/m2-frames.txt`.

**Interfaces:**
- Consumes: `run_tape` (`tests/test_engine_m2.c:2629`), the `run_natural_probe` exemplar (`:2964-3044`), the `memcmp` idiom (`:4060`), `M2_STEP_CAP`/`M2_TRANSCRIPT_CAP` (`:1568-1569`), `M2_POST_RESTART_FRAMES` (`:1603`).
- Produces: `static uint64_t run_period_end_probe(void)` → the measured completion step; the characterization T5/T6 rely on (the completion schedule + that the run currently exits there).

**Measured fact (from planning recon; re-verify):** with the stop at `:2936-2937` removed, the class-1 phase-2 clock (`set_period(mr,1,1)` → `limit=1`, carried `aux=6`, `limit+aux=7`) completes at **engine step 845** (match-relative **m=840**); the same step fires `mark_over` → one-step `OVER→POST→EXIT→run_end` (`:4161/:4174-4180`), mode `MATCH→FRONTEND`, and **no `state=12/` frame is presented**. The completion is `differing`-agnostic at this index (roll `global_157ac2=1`, not the sit-8 `==4` gate). The trajectory is byte-stable over 3 replays.

- [ ] **Step 1: Add the probe (mirror `run_natural_probe`).**

Insert a new function next to `run_natural_probe` (after `:3044`). It builds the same null config as `run_tape` (`:2632-2637`) but runs a **local** transcript and, crucially, does **not** break at the `M2_POST_RESTART_FRAMES` stop — it runs until `mr->clock_period_ended` fires (or `M2_STEP_CAP`), recording the completion step and the post-step engine mode:

```c
/* v21 (M2 phase-21 T1): the period-end feasibility probe — transcript-neutral.
 * Extends the scripted run past the v20 stop so the FU-62 clock reaches its
 * limit, and records what the run does NOW (the producers are still gated).
 * Prints nothing; never touches the golden. */
static uint64_t run_period_end_probe(void) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.tape = M2_KEYS;
  pcfg.tape_len = M2_KEYS_LEN;
  pcfg.step_ns = 10000000ull;
  fifa96_platform *plat = fifa96_platform_null_create(&pcfg);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320; ecfg.height = 240; ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  uint64_t steps = 0, match_start = 0, completion = 0;
  uint8_t exited_at_completion = 0;
  while (steps < M2_STEP_CAP) {
    uint64_t next = steps + 1;
    if (e->match != NULL && match_start == 0) match_start = next; /* :3852 asserts 5 */
    if (match_start > 0) m2_directives(e, next, match_start, NULL); /* res==NULL: no op */
    assert(fifa96_engine_step(e) == 0);
    steps++;
    if (e->match != NULL && e->match->clock_period_ended) { completion = steps; }
    if (completion != 0 && e->mode != FIFA96_ENGINE_MODE_MATCH) {
      exited_at_completion = 1; break;
    }
  }
  assert(completion != 0);            /* the period completes */
  assert(match_start == 5);
  /* Characterization (FU-230 §2.2 / the T1 verdict): the completion currently
   * exits the run in the same step — the lifecycle blocker T5 fixes. */
  assert(exited_at_completion == 1);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
  return completion;
}
```

> NB: `m2_directives` takes `struct m2_result *res`; the T1 probe passes a real `struct m2_result` (not `NULL`) exactly as `run_tape` does (`:2665`) — copy the `run_natural_probe` call convention at `:2990`. If `m2_directives` dereferences `res`, keep a local `struct m2_result res; memset(&res,0,sizeof res);`.

- [ ] **Step 2: Run the probe and record the measured completion.**

Run: `ctest --test-dir build -R test_engine_m2 --output-on-failure`
Expected: PASS; the probe reports (via a `fprintf(stderr, ...)`) `completion == 845`. If it differs, record the fresh measured value and update this plan's T6 note — **do not** invent.

- [ ] **Step 3: Add the ≥3-replay determinism check.**

In `main` (beside the `run_tape` call, `:3837`), add:

```c
  /* v21 (M2 phase-21 T1): the determinism gate — 3 replays byte-stable (spec §5 T1). */
  {
    char *r[3];
    size_t n[3];
    for (int i = 0; i < 3; i++) r[i] = run_scenario_capture(&n[i]); /* see Step 4 */
    assert(n[0] == n[1] && n[1] == n[2]);
    assert(memcmp(r[0], r[1], n[0]) == 0);
    assert(memcmp(r[1], r[2], n[0]) == 0);
    for (int i = 0; i < 3; i++) free(r[i]);
  }
```

- [ ] **Step 4: Add the replay-capture helper.**

Factor the transcript capture out of `run_tape` into `static char *run_scenario_capture(size_t *out_len)` (same null config, returns the per-step `frame=/hash=/state=` transcript). `run_tape` then calls it; the T1 check calls it 3×.

- [ ] **Step 5: Run the determinism gate.**

Run: `ctest --test-dir build -R test_engine_m2 --output-on-failure`
Expected: PASS (byte-stable). If **not** byte-stable → **STOP and report** (spec §5 T1: the fallback is a fixture-only proof with M2 unchanged).

- [ ] **Step 6: Verify the golden is untouched; commit.**

```bash
ctest --test-dir build -R test_engine_m2 --output-on-failure
./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt   # must be clean (no re-pin)
git add tests/test_engine_m2.c
git commit -m "test(engine): phase-21 T1 — the period-end feasibility probe + determinism gate"
```

---

### Task 2: T2 — the period-length regression pin (FU-223)

**Objective (spec §5 T2 + errata E3).** Confirm the engine already stages the live period length byte-exact (no producer to port) and pin it.

**Files:**
- Modify: `tests/test_engine_match_completion.c` (add `test_period_length_pin`; call it from `main`).

**Interfaces:**
- Consumes: `fifa96_match_run_set_period` (`fifa96_match_run.c:2425`), the staged defaults (`:2368..2372`), the existing `test_begin_installs_derived_periods` (`tests/test_engine_match_completion.c:71..94`).

- [ ] **Step 1: Write the failing test.**

```c
/* v21 (M2 phase-21 T2): the live period length is a match-init constant the
 * engine already stages (FU-223 §4 / spec errata E3) — no producer to port.
 * Native: [0x15881A] (regular) / [0x15881C] (extra) from [0x14C1D1]
 * (FUN_0003749C 0x375E4/ED, FUN_000886D4 0x88710/30, FUN_0004B508 0x4B514/1B);
 * the default half (settings[0x0E]=0 -> table 0x37170[0]=2 min) = 2*0x3C/2*0x14.
 * Mutation-RED: changing DEFAULT_PERIOD_SECONDS (match_run.h:72) aborts. */
static void test_period_length_pin(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run *mr = &f.engine->match_store; /* the fixture's run store */
  assert(fifa96_match_run_begin(mr, /*selector=*/0) == 0);
  assert(mr->state.period_length == 120u);   /* [0x15881A] = 2*0x3C */
  assert(mr->state.extra_length  == 40u);    /* [0x15881C] = 2*0x14 */
  drop_fixture(f);
}
```

> Use the exact run-store accessor the sibling tests use (see `test_begin_installs_derived_periods` at `:71..94` for the begin call shape); mirror it rather than inventing a field name.

- [ ] **Step 2: Run it to verify it fails (before the pin exists).**

Run: `ctest --test-dir build -R test_engine_match_completion --output-on-failure`
Expected: FAIL/compile error (function not yet called / name mismatch) — then wire it into `main`.

- [ ] **Step 3: Add the call to `main`; run.**

Add `test_period_length_pin();` beside the other calls (`:302-313`). Run: `ctest --test-dir build -R test_engine_match_completion --output-on-failure`
Expected: PASS.

- [ ] **Step 4: Mutation-RED check (manual, recorded).** Temporarily change `DEFAULT_PERIOD_SECONDS` / the staged value and confirm the assert aborts; revert.

- [ ] **Step 5: Commit.**

```bash
git add tests/test_engine_match_completion.c
git commit -m "test(engine): phase-21 T2 — the period-length pin (FU-223, no producer)"
```

---

### Task 3: T3 — the roll + extra-time-flag staging pins (FU-224)

**Objective (spec §5 T3 + errata E1).** Pin the two live cells (`mr->global_157ac2` roll, `mr->global_157ac0` OL-85 flag) and their staging; the front-end producers are unreachable (FU-224 §4.3) so the stand-in is the already-committed zero — no functional change.

**Files:**
- Modify: `tests/test_engine_match_frame.c` (add `test_roll_flag_staged_zero` + `test_phase_drive_roll4_flag_differing`; call from `main`).

**Interfaces:**
- Consumes: `fifa96_match_run_init` (`fifa96_match_run.c:2034-2035` zeroes the cells), `fifa96_match_run_phase_drive` (`:2584`), the roll (`:2643`), the flag writer (`:2618-2619`).

- [ ] **Step 1: Write the failing tests.**

```c
/* v21 (M2 phase-21 T3): the roll + extra-time-flag cells (FU-224 §1/§3/§4.3).
 * FE producers 0x38990/0x389A4 are unreachable -> the stand-in is the native
 * BSS default (zero), already staged at init/begin. Mutation-RED: seeding a
 * non-zero stand-in, or a wrong roll, aborts. */
static void test_roll_flag_staged_zero(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(mr.global_157ac2 == 0u);   /* [0x157AC2], FU-224 §1 */
  assert(mr.global_157ac0 == 0u);   /* [0x157AC0], FU-224 §1 */
}

static void test_phase_drive_roll4_flag_differing(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.state.period = 4;                 /* completed index 3; past half-time */
  mr.state.phase = 2;                  /* class 1 */
  mr.phase_machine.side_controlled = 0;
  mr.score[0] = 2u; mr.score[1] = 1u;  /* differing -> W2 raises the flag */
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.global_157ac2 == 4u);      /* the roll (match_run.c:2643) */
  assert(mr.global_157ac0 == 1u);      /* the OL-85 flag (match_run.c:2618, differing) */
}
```

- [ ] **Step 2: Run to verify failure, then wire into `main` and run.**
Run: `ctest --test-dir build -R test_engine_match_frame --output-on-failure` — expected PASS after wiring.

- [ ] **Step 3: Mutation-RED check.** Flip `differ` (or a stand-in seed) and confirm the abort; revert.

- [ ] **Step 4: Commit.**

```bash
git add tests/test_engine_match_frame.c
git commit -m "test(engine): phase-21 T3 — the roll + extra-time-flag staging pins (FU-224)"
```

---

### Task 4: T4 — the live mode-0 cadence (FU-225 D10 / errata E2)

**Objective (spec §5 T4 + errata E2).** Make the mode-0 producer (`fifa96_match_run_period_setup(mr, 0)`, native `FUN_0004B02C(0)` at `0x4950A`) run **every frame** (native cadence) instead of only on an index-4 completion — the edit that makes the sit-1 else-arm reachable. There is **no** `period_setup_live` flip (already `1` at `:2225`). FU-225 D2–D4 stand-ins prescribe no non-zero value (zero = native BSS default; no change).

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` — insert the per-frame call at `:2591-2593`; **delete** the guarded call at `:2654-2657` (native-faithful form A, FU-225 §4).
- Modify: `tests/test_engine_m2.c` — add `run_phase21_riders` (cadence + chooser stand-in rider) and re-pin `run_phase19_riders` blocks 3/4/5 if the one-frame cadence shifts them.

**Interfaces:**
- Produces: `fifa96_match_run_period_setup(mr, 0)` on every `fifa96_match_run_phase_drive` call; the sit-1 (half-time) arm reachable; the bit-8 extra-time gate (`:3214`) live per-frame.
- Depends-on: nothing (T5 depends on this).

- [ ] **Step 1: Write the failing rider.**

Add to `tests/test_engine_m2.c` after `run_phase19_riders` (`:3789`); register in `main` (`:3832`):

```c
/* v21 (M2 phase-21 T4): the mode-0 per-frame cadence + the chooser stand-ins.
 * Mutation-RED: reverting the per-frame mode-0 call (FU-225 D10) leaves the
 * tail dormant, so the bit-8 gate never raises the index and assert 1 aborts. */
static void run_phase21_riders(void) {
  /* 1. CADENCE (FU225-L5 / D10): the mode-0 common tail runs on a plain
   *    (non-completion) frame; the bit-8 gate (fifa96_match_run.c:3214) is the
   *    observable — with the per-frame call the index rolls to 4. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.phase = 2;                 /* class 1, no completion */
    mr.input_range[0] = 0x0100u;        /* the 0x4B0CB TEST AH,1 condition */
    assert(fifa96_match_run_phase_drive(&mr) == 0);
    assert(mr.global_157ac2 == 4u);
  }
  /* 2. STAND-INS (FU-225 D2-D4): the zero stand-ins yield the derived chooser
   *    arm (extra_time + period>=4 + d8<=d9 -> edx 1 -> phase 0x13/act 0xC).
   *    Mutation-RED: an inverted d8/d9 compare or probe order aborts. */
  {
    fifa96_action_phase_period_end_out o;
    uint8_t probe[4] = { 0u, 0u, 0u, 0u };
    assert(fifa96_action_phase_period_end(4u, 1u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
                                          probe, &o) == FIFA96_OK);
    assert(o.phase == 0x13u && o.act == 0x0Cu && o.side == 0u);
  }
}
```

- [ ] **Step 2: Run to verify it fails.**

Run: `ctest --test-dir build -R test_engine_m2 --output-on-failure`
Expected: FAIL at rider 1 (`global_157ac2 != 4` — the per-frame call is absent).

- [ ] **Step 3: Implement the per-frame cadence.**

In `src/fifa96_engine/fifa96_match_run.c`, inside `fifa96_match_run_phase_drive` (opens `:2584`), after the null guard (`:2591-2593`), insert:

```c
  /* FU-225 §4 / D10 (T4): the native `FUN_0004B02C(mode 0)` runs EVERY frame
   * (FUN_000493A0 loop 0x4950A), before the clock; its own gates
   * (state.phase == 0x0C, global_157ac2 == 4) select the sit-1/sit-8 arm.
   * Hoisting it here is the per-frame cadence; the index-4 completion call
   * below is removed as redundant (native-faithful, one call per frame). */
  (void)fifa96_match_run_period_setup(mr, 0);
```

Then **delete** `:2654-2657` (the `if (mr->global_157ac2 == 4u) (void)fifa96_match_run_period_setup(mr, 0);` guarded call) and rewrite the stale comment block `:2644-2653`.

- [ ] **Step 4: Run the focused tests.**

Run: `ctest --test-dir build -R 'test_engine_m2|test_engine_match_frame|test_engine_match_completion' --output-on-failure`
Expected: PASS. If `run_phase19_riders` blocks 3/4/5 (sit-8 same-frame assertions, `:3658-3671`, `:3692-3709`, `:3746-3766`) shift to the next-frame cadence, re-pin them to a second `phase_drive` call and record why.

- [ ] **Step 5: Verify the goldens are unmoved; run the gate.**

```bash
./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt   # clean (tape reaches no completion)
./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt   # clean
make check
```

- [ ] **Step 6: Mutation-RED check.** Remove the inserted call (or re-add the `== 4u` guard) and confirm rider 1 aborts; restore.

- [ ] **Step 7: Commit.**

```bash
git add src/fifa96_engine/fifa96_match_run.c tests/test_engine_m2.c
git commit -m "feat(engine,fu225): the live per-frame mode-0 cadence (phase-21 T4)"
```

> **Fallback (only if form A proves disruptive):** use FU-225 §4 form B — keep the guarded call and add the per-frame call (double-runs the tail on completion frames via the `global_157a4c != 1` guard). Record the deviation + reason in the task report.

---

### Task 5: T5 — the half-time route + the `mark_over` re-point (FU-227/FU-228)

**Objective (spec §5 T5 + errata E4).** Make the first (half-time) completion continue the match instead of exiting: re-point the `mark_over` trigger at the FU-228 native condition (end word decided **and** past half-time), and assert the sit-1 half-time route and the ET-entry route run end-to-end.

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c:4161-4167` (the `mark_over` trigger only).
- Modify: `tests/test_engine_match_completion.c` (add the half-time route + `mark_over` tests; update the 3 named lifecycle pins).
- Modify: `tests/test_engine_match_frame.c` (add the chooser half-time select; update `:493` and `:717`).

**Interfaces:**
- Consumes: the T4 live mode-0 call; the sit-1 else-arm (`:3199-3209`); the phase-0x17 row (`fifa96_match_handlers.c:5235`); the lifecycle (`fifa96_match_lifecycle_*`).
- Produces: a first completion that continues (`state.phase` → `1` kickoff), a past-half-time flag-clear completion that still exits.

- [ ] **Step 1: Write the failing half-time route test.**

In `tests/test_engine_match_completion.c` (mirror the loop-backed `TICKS_PER_SECOND` pattern `:233..266`):

```c
/* v21 (M2 phase-21 T5): the half-time completion runs the sit-1 route live
 * (FU-227 §5). Mutation-RED: restoring the `== 4u` guard at
 * fifa96_match_run.c:2654 leaves the sit-1 arm dormant -> state.phase stays
 * 0x0C, act.id stays 0x0B -> the state.phase == 1 assert aborts. */
static void test_half_time_completion_runs_sit1_kickoff(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run *mr = <fixture run store>;   /* see :233 pattern */
  assert(fifa96_match_run_begin(mr, 0) == 0);
  assert(fifa96_match_run_set_period(mr, 1, 1) == 0);
  mr->state.phase = 2;                                  /* class 1: clock runs */
  for (int i = 0; i < 200 && mr->state.period == 0; i++) {
    assert(fifa96_match_run_phase_drive(mr) == 0 || 1); /* advance to completion */
  }
  /* after the completion frame: the sit-1 arm ran, phase-0x17 stage 0 wrote the kickoff phase */
  assert(mr->state.period == 1u);
  assert(mr->global_157ac2 == 1u);                      /* the roll */
  assert(mr->act.id == 1u);                             /* sit-1 installed act 1 */
  assert(mr->state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE); /* == 1u, kickoff_entry */
  drop_fixture(f);
}
```

> The exact loop driver (which frame entry point + how many granted frames) mirrors `test_loopbacked_tiebreak_completion` (`:233..266`), which already reaches a real clock completion. Reuse its driver; the delta is the half-time assertion set.

- [ ] **Step 2: Add the `mark_over` boundary test.**

```c
/* v21 (M2 phase-21 T5): the lifecycle exits only on a PAST-half-time,
 * end-word-decided completion (FU-228 §4.2). Mutation-RED: the raw
 * period_ended gate fires at half-time -> the ACTIVE assert aborts. */
static void test_mark_over_only_past_half_time(void) {
  /* (a) half-time (index 1, flag clear): no exit */
  { struct fifa96_match_run mr; fifa96_match_run_init(&mr);
    mr.state.period = 1; mr.state.phase = 2; mr.clock_period_ended = 1;
    mr.phase_machine.side_controlled = 0;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    /* run the frame body's lifecycle consume (see :121/:196 patterns) */
    assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE); }
  /* (b) past-half-time + flag clear + end decided: exit (OVER) */
  { struct fifa96_match_run mr; fifa96_match_run_init(&mr);
    mr.state.period = 3; mr.state.phase = 2; mr.clock_period_ended = 1;
    mr.global_157ac2 = 3u; mr.global_157ac0 = 0u;   /* end word decided, flag clear */
    mr.phase_machine.side_controlled = 0;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER); }
}
```

- [ ] **Step 3: Run to verify failure.**

Run: `ctest --test-dir build -R 'test_engine_match_completion|test_engine_match_frame' --output-on-failure`
Expected: FAIL (the current raw `period_ended` trigger fires `mark_over` at half-time).

- [ ] **Step 4: Implement the `mark_over` re-point.**

In `src/fifa96_engine/fifa96_match_run.c:4161-4167`, replace the raw-`period_ended` gate with the FU-228 §4.2 derived condition (drive the exit from the end-of-match condition; do **not** port `FUN_0008BAF0`):

```c
  /* FU-228 §4.2 (T5): the native `mark_over` fires only when the end word is
   * decided AND the period index is past half-time (0x4B1C6 CMP [0x157AC2],2;
   * 0x4B244). The end word is set on a phase-0x0C screen with the extra-time
   * flag [0x157AC0] CLEAR (0x8BCBF/0x8BCD9); a flag-set ET entry does not end.
   * Derived from the live cells (no [0x158822] cell; FU228-L6 caveat). */
  {
    int past_half_time = (mr->global_157ac2 >= 2u);
    int end_word_decided = (mr->state.phase == 0x0Cu && mr->global_157ac0 == 0u);
    if (period_ended && past_half_time && end_word_decided &&
        !fifa96_match_lifecycle_should_exit(&mr->lc)) {
      rc = fifa96_match_lifecycle_mark_over(&mr->lc);
      if (rc != 0) return rc;
    }
  }
```

- [ ] **Step 5: Run the focused tests; update the 3 lifecycle pins.**

Run: `ctest --test-dir build -R 'test_engine_match_completion|test_engine_match_frame' --output-on-failure`
Update the now-changed pins per FU-228 §4.2 / FU228-L4: `tests/test_engine_match_completion.c:121` (`test_period_end_resolve_returns_to_frontend`), `:160` (`test_staged_exit_survives_period_end`), `:196` (`test_live_period_end_exits_to_frontend`), `:233` (`test_loopbacked_tiebreak_completion`), and `tests/test_engine_match_frame.c:493`/`:717` — each re-derived to a **past-half-time, flag-clear** completion for the exit positive, and continuation at half-time.

- [ ] **Step 6: Add the chooser half-time select test** (sibling of `test_phase_drive_extra_time_tiebreak` `:553`) in `tests/test_engine_match_frame.c`:

```c
/* v21 (M2 phase-21 T5): a first completion selects the sit-1 (half-time) arm.
 * Mutation-RED: a wrong arm leaves act.id == 0x0B. */
static void test_phase_drive_half_time_selects_act1(void) {
  struct fifa96_match_run mr; fifa96_match_run_init(&mr);
  mr.state.period = 1; mr.state.phase = 2; mr.state.frame_delta = 2;
  mr.phase_machine.side_controlled = 0; mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.global_157ac2 == 1u);
  assert(mr.act.id == 1u);        /* act 1 is install-only; dispatched by act_tick */
  assert(mr.act.stage == 0u);
}
```

- [ ] **Step 7: Verify the goldens are unmoved; run the gate.**

```bash
./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt   # clean (T6 owns the re-pin)
./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt   # clean
make check
```

- [ ] **Step 8: Commit.**

```bash
git add src/fifa96_engine/fifa96_match_run.c tests/test_engine_match_completion.c tests/test_engine_match_frame.c
git commit -m "feat(engine,fu228): the half-time route + the mark_over re-point (phase-21 T5)"
```

> **Do not** re-pin the golden in T5 (T6 owns it). The v20 tape reaches no completion (`tests/test_engine_m2.c:3860 assert(res.exit_step < 0)`), so T5 is tape-inert.

---

### Task 6: T6 — the tape extension + re-pin + acceptance (FU-230)

**Objective (spec §5 T6/§6 + errata E1).** Extend the M2 tape to play half-time and a **differing-score** regulation end → extra-time entry; re-pin M2 with the written reason + frame diff; add the 7 mutation-RED rider classes; update the docs.

**Files:**
- Modify: `tests/test_engine_m2.c` (directives + `m2_result` fields + the completion window + `run_phase21_riders` + `main` anchors + the provenance block + the file header).
- Modify: `tests/golden/engine/m2-frames.txt` (re-pin v20 → v21).
- Modify (only if the census moves; expected none): `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md`.

**Interfaces:**
- Consumes: T1's measured completion schedule; the T4 cadence; the T5 `mark_over` re-point.
- Produces: a v21 golden; the phase-21 acceptance strata.

- [ ] **Step 1: Add the directive macros + tail bound.**

After `tests/test_engine_m2.c:1592`:
```c
#define M2_M_HALF_TIME <T1-measured m>   /* period 0 -> 1 (half-time); FU230-L1 */
#define M2_M_ET_ENTRY  <T1-measured m>   /* period 1 -> 2 (reg end -> ET entry); FU230-L1 */
```
Near `:1603`: `#define M2_POST_PERIOD_END_FRAMES <T1-measured tail>`.
>The `m` values are **T1-owned** (FU230-L1); take them from the T1 probe's measured schedule, never invent.

- [ ] **Step 2: Add the two directive bodies** between the `m2_directives` M2_M_FINAL block `}` (`:2623`) and the function `}` (`:2624`):

```c
    else if (m == M2_M_HALF_TIME) {
      /* v21 (T6): the period-0 completion -> the sit-1 half-time route. */
      assert(mr->global_157ac0 == 0u);          /* no differing score yet */
      assert(mr->global_157ac2 == 1u);          /* the roll */
      assert(mr->state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE); /* sit-1 kickoff */
      mr->half_step = next;                      /* the sample id */
    } else if (m == M2_M_ET_ENTRY) {
      /* v21 (T6): the DIFFERING reg-end completion -> extra-time entry (E1). */
      assert(mr->global_157ac0 == 1u);          /* W2 0x8B2AC, differing */
      assert(mr->global_157ac2 == 4u);          /* the roll to the sit-8 gate */
      assert(mr->act.id == 7u);                 /* situation 8 -> act 7 */
      assert(mr->state.phase == 0x10u);         /* phase-0x1D arm 0 write */
      mr->et_step = next; mr->et_entry_step = steps; mr->et_phase = mr->state.phase;
    }
```

> Match the exact field/accessor names the existing directive blocks use (they reference `mr`, `res`, `next`, `steps` — copy the surrounding block's convention verbatim; `mr` is the run pointer in that scope).

- [ ] **Step 3: Add the `struct m2_result` fields** after `:1934`: `uint64_t half_step, et_step, et_entry_step; uint8_t et_phase;`.

- [ ] **Step 4: Replace the stop with the completion window** at `:2936-2937`:

```c
    /* v21 (T6): keep running past the v20 restart stop until the ET-entry anchor
     * (or the hard bound), so the two period completions play. */
    if (res->restart_step > 0 && res->et_entry_step == 0 &&
        steps - res->restart_step >= M2_POST_RESTART_FRAMES) break;   /* pre-ET guard */
    if (res->et_entry_step > 0 && steps - res->et_entry_step >= M2_POST_PERIOD_END_FRAMES) break;
```
Keep `assert(steps < M2_STEP_CAP);` (`:2939`). Raise `M2_STEP_CAP`/`M2_TRANSCRIPT_CAP` (`:1568-1569`) only if the measured `N` exceeds them (FU230-L4).

- [ ] **Step 5: Run to verify the extension reaches ET entry; capture the measured `N`.**

Run: `./build/test_engine_m2 > /tmp/m2-v21.txt; wc -l /tmp/m2-v21.txt`
Expected: `N` in the FU-230 §2.3 range (~1050–1600; higher if a cutscene enters). If it does not reach the ET anchor → adjust the `m` values from the measured schedule (T1-owned).

- [ ] **Step 6: Add the `run_phase21_riders` rider classes** after `:3789` (mirror `run_phase19_riders`, `:3597`), the 7 classes FU-230 §3.3 names: (1) half-time→sit-1→act 1→phase 0x17; (2) reg-end→flag→act 0xC; (3) **tie does NOT arm ET** (the E1 polarity discriminator); (4) roll→index 4; (5) ET-entry slot deref→phase 0x10; (6) clock schedule; (7) additivity/determinism. Register `run_phase21_riders();` in `main` after `run_phase19_riders()` (`:3832`).

- [ ] **Step 7: Add the `main` anchors** (after `:3927`): `res.half_step > res.restart_step`, `res.et_entry_step > res.et_step > res.half_step`, `res.et_phase == 0x10u`.

- [ ] **Step 8: Re-pin the golden (the methodology).**

```bash
cp tests/golden/engine/m2-frames.txt /tmp/m2-v20.txt
./build/test_engine_m2 > tests/golden/engine/m2-frames.txt
head -792 tests/golden/engine/m2-frames.txt | cmp - /tmp/m2-v20.txt   # additivity: clean
cmp /tmp/m2-v20.txt tests/golden/engine/m2-frames.txt                 # first differing line
diff <(cut -d' ' -f3- /tmp/m2-v20.txt) <(cut -d' ' -f3- tests/golden/engine/m2-frames.txt)  # state= isolation
diff <(cut -d' ' -f2   /tmp/m2-v20.txt) <(cut -d' ' -f2   tests/golden/engine/m2-frames.txt)  # hash= isolation
```
If the 792-prefix is **not** clean, document the shifted region **line-by-line with the reason** (spec §6) — never silently. Any *unexplained* drift is a regression to investigate, never a re-pin.

- [ ] **Step 9: Determinism gate.** Run `test_engine_m2` ≥3× and `cmp` byte-for-byte. Not stable → STOP (spec §5 T1).

- [ ] **Step 10: Add the v21 provenance block** between `:1542` and `:1543` (template: the v14 re-pin block, `:1154-1213`), and bump the file header `:1-13` (`v20` → `v21`). Use this written reason:

> The M2 golden is re-pinned for phase-21 (the live period-end spine; FU-223…FU-230, T6). Reason: the tape now drives the live FU-62 clock through **two period completions** — period 0 → 1 (half-time) and period 1 → 2 (regulation end) — and the reg-end completion installs the OL-85 extra-time arm (`[0x157AC0]` W2 `0x8B2AC`, raised because the reg-end score **differs**: errata E1). The acceptance stops at **extra-time entry** (spec §2 non-goal: no full ET match). Frame diff (measured with the ISO present): lines 1..792 byte-identical (`head -792 new | cmp - old` clean); first appended line is 793; … The intended live continuation, not a regression. Regenerated with the ISO present (`./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`); M1 unmoved (`09b726b7…`); old M2 `441415fc…` (v20, 792 lines) → new M2 `<sha>` (v21, `<N>` lines).

- [ ] **Step 11: Verify M1 unmoved; update the docs; run the gate.**

```bash
./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt   # MUST be clean
make check
```
Update `docs/ENGINE.md` / `docs/PROGRESS.md` / `README.md` / `AGENTS.md` **only if** the wired count moved (expected: none — still 55/80 = 68%); record the re-pin + the smoke in `docs/PROGRESS.md`.

- [ ] **Step 12: Smoke (`DISPLAY=:1`) + commit.**

Run `make game` on a display and record an honest smoke note (the live reach: the clock → the completions). Then:

```bash
git add tests/test_engine_m2.c tests/golden/engine/m2-frames.txt docs/ENGINE.md docs/PROGRESS.md README.md AGENTS.md
git commit -m "test(engine): phase-21 T6 — the live period-end acceptance tape v21 (re-pin)"
```

---

## Self-review (spec coverage)

- Spec §5 T1 → Task 1; T2 → Task 2; T3 → Task 3; T4 → Task 4; T5 → Task 5; T6 → Task 6. ✓
- Errata E1 (differing reg end) → Task 6 (the ET-ENTRY directive + rider 3) + the T1/T3 confirmation. E2 (no flip) → Task 4. E3 (staged length) → Task 2. E4 (no end machine) → Task 5. ✓
- Non-goals: no audio (FU-229 OUT), no full ET match end (T6 stops at ET entry), no dead-gate synthesis, no new front-end art. ✓
- M1 immutable / M2 re-pins only at T6. ✓
- The T4/T5 boundary (the mode-0 call-site change is T4's; T5 owns only `mark_over`) is a recorded ruling — one task owns the call site. ✓
