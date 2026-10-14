# FIFA 96 M2 Phase-22 — The End Spine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the live match end spine real on the M2 tape — the extra-time halves clock to their end, the end word `[0x158822]` (1 = regulation-over, 2 = extra-time-over) fires, the match-over screen dwells, and the run returns to the front end — replacing the phase-21 derived proxy with the ported terminal.

**Architecture:** Four engine changes on top of the phase-21 re-point: (T1) the period-over **sit-8 re-arm** so the ET half plays/clocks in class-1 **phase 2** (FU-239); (T2) the `FUN_0008BAF0` end machine as a new `mr->end_word_158822` cell with its three arms; (T3) the `mark_over` tail keyed to that cell; (T4) the ET-over screen dwell + the return path. Then (T5) the M2 tape extends past ET entry to the terminal + the front-end return, and is re-pinned v21→v22; (T6) the additivity gate + the mutation-RED riders + the errata.

**Tech Stack:** C (C99), the `fifa96_loader` FU-62 clock + `fifa96_engine` match spine, the null backend as determinism source of truth; CTest under ASan/UBSan `-fno-sanitize-recover=all`.

**Spec:** `docs/superpowers/specs/2026-10-13-fifa96-m2-phase22-end-spine-design.md` (argue from it; it travels with this plan).

**Design authority (frozen recon — read before touching code):**
- `docs/ghidra/FU239_et_rearm_and_live_phase.md` — **the ET re-arm + the live phase 2 (T1's authority)**.
- `docs/ghidra/FU231_end_machine.md` (the three terminal arms), `FU236_regulation_end_route.md`, `FU232_frame_loop_lifecycle.md`, `FU233_extra_time_halves.md`, `FU234_front_end_return.md`, `FU235_match_over_screen.md`, `FU237_extra_time_terminal.md`, `FU238_tape_extension_v22.md`.

## Global Constraints

- Every address/byte claim cites a disassembly or a reconciled raw-byte read; unprovables become numbered legs, never asserted. (AGENTS.md evidence rules.)
- The M1 golden `tests/golden/engine/m1-frames.txt` (`09b726b7…`, 691 lines) is **immutable** — stop and report if it moves.
- The M2 golden `tests/golden/engine/m2-frames.txt` re-pins only with a written reason + frame diff; the v21 **2522-line prefix must stay byte-identical** (FU-238 §2.5).
- Determinism source of truth is the null backend (`src/fifa96_engine/platform_null.c`).
- Engine edits stay in `fifa96_engine`/`fifa96_loader`; the ISO is read-only.
- Build/test gate: `make check` (strict `-Wall -Wextra -Werror` + full CTest). Single: `ctest --test-dir build -R <name> --output-on-failure`; binaries run from the repo root.
- Commit types `feat|fix|docs(fuNNN)|test|chore`; work on `main`; `git push origin main` per task.
- Census: no phase **row** is wired in this phase (rows `0x13`/`0x14` stay NULL, FU233-L1) → the "N/M action rows wired" headline stays **55/80 = 68%**; do not touch the README title unless a task explicitly wires a row.

## File Structure

- `src/fifa96_engine/fifa96_match_run.c` — the spine: the period-over re-arm / ET arm (T1), the end machine + the `mark_over` tail (T2/T3), the ET-over dwell + the return (T4). One responsibility: the match frame lifecycle.
- `include/fifa96_engine/fifa96_match_run.h` — the new `mr->end_word_158822` cell + any new API (T2).
- `src/fifa96_engine/fifa96_match_run.h`-adjacent act state — the ET-over dwell timer (`mr->act.timer` = `word[0x158818]`) reuse (T4); no new file.
- `tests/test_engine_match_completion.c` — the end-word/`mark_over` unit home (T2/T3).
- `tests/test_engine_match_frame.c` — the frame/phase home (T1/T4).
- `tests/test_match_state.c` — the clock home (T1, characterization only).
- `tests/test_engine_m2.c` — the acceptance tape + the in-test gate (T5/T6).
- `tests/golden/engine/m2-frames.txt` — the re-pinned golden (T5).

---

### Task 1: The extra-time half live (the sit-8 re-arm → phase 2)

**Authority:** FU-239 §1/§3/§4. The ET half currently parks in class-0 phase `0x13` and the clock never ticks (the T1 probe stall). The native re-arms at period-over via `FUN_0004B02C(mode 0)` → `FUN_00088860` (gate phase `0x0C`) → `sit-8` when `dword[0x157AC2]==4` → the ET half runs in class-1 **phase `2`**.

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` (the period-over re-arm ~`:3207-3217`; the ET entry `phase_drive` ~`:2584-2696`; the act-`0xC` path).
- Test: `tests/test_engine_match_completion.c` (+ `tests/test_engine_match_frame.c`).

**Interfaces:**
- Consumes: `mr->state.period`, `mr->state.phase`, `mr->global_157ac2`, `mr->extra_length` (from phase-21).
- Produces: a live ET half — after entry the engine reaches `phase==2u` (class 1) and `mr->state.period` advances `2→3` on `extra_length`; a helper `fifa96_match_run_period_over_rearm(mr)` that runs the sit-1 (half-time) **or** sit-8 (ET) arm off the phase-`0x0C` gate.

- [ ] **Step 1: Characterization test — the ET half must clock.**

In `tests/test_engine_match_completion.c`, add (mirror `test_loopbacked_tiebreak_completion` `:280`, driving the clock with `make_fixture`):

```c
/* FU-239: the extra-time half is class-1 phase 2 and must complete. */
static void test_extra_time_half_completes(void) {
  struct fixture f; make_fixture(&f);
  f.mr.state.period = 2u;              /* the first extra-time half */
  f.mr.state.extra_length = 1u;        /* 1 s, as M2_M_MECH sets it */
  fifa96_match_run_period_over_rearm(&f.mr);   /* the ET arm */
  assert(f.mr.state.phase == 2u);      /* class 1 -- the live ET phase (FU-239 §3) */
  int ended = 0;
  for (int i = 0; i < 200 && !ended; i++) fifa96_match_state_tick(&f.mr.state, 0, &ended);
  assert(ended == 1 && f.mr.state.period == 3u);   /* the ET half completed */
  drop_fixture(&f);
}
```
Call it from `main` beside the other tests.

- [ ] **Step 2: Run it — expect failure.**

Run: `ctest --test-dir build -R test_engine_match_completion --output-on-failure`
Expected: FAIL — `fifa96_match_run_period_over_rearm` undefined (or `phase != 2`, the ET park).

- [ ] **Step 3: Implement the re-arm helper + the sit-8 arm.**

In `src/fifa96_engine/fifa96_match_run.c`, beside the existing phase-`0x0C` gate (`:3207-3217`), factor the period-over re-arm and add the ET discriminator:

```c
/* FU-239 §1: the period-over re-arm dispatcher (native FUN_0004B02C(mode 0) ->
 * FUN_00088860). Gate: phase 0x0C. Discriminator: dword[0x157AC2]==4 -> the
 * extra-time half (sit-8, act 7, row 0x1D); else the half-time half (sit-1,
 * act 1, row 0x17). The live half then runs in class-1 phase 2 (FU-239 §3). */
void fifa96_match_run_period_over_rearm(fifa96_match_run *mr) {
  mr->state.phase = 2u;          /* sit-8/sit-1 both reach the class-1 play phase 2 */
  mr->state.period_seconds = 0u;
  /* 0x8ABCF/0x8AEBA install act 1/7; the kickoff restart writes phase 1/0x10
     (class 0) then phase 2 (0x8AEFA MOV EAX,2). The engine collapses the
     staging frames to the play phase; the exact route is FU239-L1. */
}
```
**No live wiring in T1 (controller ruling, the T1↔T4 conflict):** add the helper + the characterization test **only**. Do **not** modify `phase_drive` or the half-time call site — a live re-arm at the ET chooser fires at engine step 2502 and changes M2 transcript frames 2503–2522 (`state=19`→`state=2`), breaking T5's byte-identical v21 prefix and the ET-entry riders (`tests/test_engine_m2.c:2780`, `:3900`). **T4 owns the ET-entry screen dwell + the live re-arm call**, which fires only after the dwell (step > 2522).

- [ ] **Step 4: Run the test — expect pass.**

Run: `ctest --test-dir build -R test_engine_match_completion --output-on-failure`
Expected: PASS.

- [ ] **Step 5: Guard the whole suite + commit.**

Run: `make check` — Expected: green (the phase-21 lifecycle tests still pass; no phase row wired → census unchanged).

```bash
git add src/fifa96_engine/fifa96_match_run.c tests/test_engine_match_completion.c
git commit -m "feat(engine): the extra-time half live via the sit-8 re-arm (phase 2)"
```

---

### Task 2: The end machine (FUN_0008BAF0 → the end word `[0x158822]`)

**Authority:** FU-231 §3/§4, FU-236, FU-239 §5. Three arms: regulation-over `1` (the flag-clear phase-`0xC` terminal `0x8BCD9`), ET-over `2` (the `0x13`/`0x14` dwell past the timer gate → `0x8BD8A`/`0x8BDD4`), and the no-end-word screen `[0x105FFC]=5` (`0x37F54`, no engine cell → leg).

**Files:**
- Modify: `include/fifa96_engine/fifa96_match_run.h` (the cell), `src/fifa96_engine/fifa96_match_run.c` (init `:2035`/begin `:2157` + the machine).
- Test: `tests/test_engine_match_completion.c`.

**Interfaces:**
- Consumes: `mr->state.phase`, `mr->state.period`, `mr->global_157ac0`, `mr->global_157ac2`, `mr->act.timer` (= `word[0x158818]`).
- Produces: `uint16_t fifa96_match_run_end_word(const fifa96_match_run *mr)` returning `0|1|2`; the cell `mr->end_word_158822`.

- [ ] **Step 1: Failing test — the three arms.**

```c
static void test_end_word_arms(void) {
  struct fixture f; make_fixture(&f);
  /* regulation-over: the flag-clear phase-0xC terminal (FU-231 §3 arm 1) */
  f.mr.state.phase = 0x0Cu; f.mr.global_157ac0 = 0u; f.mr.global_157ac2 = 2u;
  assert(fifa96_match_run_end_word(&f.mr) == 1u);
  /* extra-time-over: the phase-0x13/0x14 dwell past the timer gate (FU-239 §5) */
  f.mr.state.phase = 0x14u; f.mr.act.timer = 0x4B1u;
  assert(fifa96_match_run_end_word(&f.mr) == 2u);
  /* no end word: still 0 */
  f.mr.state.phase = 0x02u;
  assert(fifa96_match_run_end_word(&f.mr) == 0u);
  drop_fixture(&f);
}
```

- [ ] **Step 2: Run — expect fail (undefined).**
Run: `ctest --test-dir build -R test_engine_match_completion --output-on-failure`

- [ ] **Step 3: Implement.**

Add to `fifa96_match_run.h` (beside `global_157ac0`, `:622`): `uint16_t end_word_158822;` and stage `0u` at init (`:2035`) and begin (`:2157`). In `fifa96_match_run.c`:

```c
/* FU-231 §3: the FUN_0008BAF0 end machine. word 1 = regulation-over (the
 * flag-clear phase-0xC terminal, 0x8BCD9); word 2 = extra-time-over (the
 * phase-0x13/0x14 dwell past word[0x158818]>0x4B0, 0x8BD8A/0x8BDD4); 0 = none.
 * The no-ET screen 0x37F54 ([0x105FFC]=5) has no engine cell -> FU-231 leg. */
uint16_t fifa96_match_run_end_word(const fifa96_match_run *mr) {
  if (mr->state.phase == 0x0Cu && mr->global_157ac0 == 0u) return 1u;
  if ((mr->state.phase == 0x13u || mr->state.phase == 0x14u) &&
      mr->act.timer > 0x4B0u) return 2u;
  return 0u;
}
```
Stage it: in the frame body, when `mr->end_word_158822 == 0u`, set it from `fifa96_match_run_end_word(mr)` **gated on `global_157ac2 >= 2u` and `mr->act.timer >= 0x78u`** (FU-234-L7: never the raw `period_ended`, so the half-time phase-`0xC` write cannot fire the word early).

- [ ] **Step 4: Run — expect pass.** `ctest --test-dir build -R test_engine_match_completion --output-on-failure`

- [ ] **Step 5: Commit.**
```bash
git add include/fifa96_engine/fifa96_match_run.h src/fifa96_engine/fifa96_match_run.c tests/test_engine_match_completion.c
git commit -m "feat(engine): the FUN_0008BAF0 end machine -> the end word cell"
```

---

### Task 3: The frame-loop re-point (`mark_over` keyed to the end word)

**Authority:** FU-232 §2/§4, FU-237. `mark_over` iff the end word is set **and** `global_157ac2 >= 2` — replacing the phase-21 proxy `end_word_decided` (`fifa96_match_run.c:4185`), which excludes the ET-over.

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` (the `mark_over` tail `:4177-4192`).
- Test: `tests/test_engine_match_completion.c` (`test_mark_over_only_past_half_time` `:360` + a new ET-over pin).

**Interfaces:**
- Consumes: `mr->end_word_158822` (T2), `mr->global_157ac2`, `mr->global_157ac0`.
- Produces: the live lifecycle trigger — `run_step` compresses OVER→POST→EXIT→`run_end`.

- [ ] **Step 1: Failing test — the ET-over marks over; the half-time does not.**

```c
static void test_mark_over_on_end_word(void) {
  struct fixture f; make_fixture(&f);
  /* half-time: phase 0x0C + flag-clear but the word was blocked (FU-234-L7) */
  f.mr.state.phase = 0x0Cu; f.mr.global_157ac0 = 0u; f.mr.global_157ac2 = 2u;
  f.mr.end_word_158822 = 0u;
  assert(fifa96_match_run_should_mark_over(&f.mr) == 0);
  /* extra-time-over */
  f.mr.end_word_158822 = 2u;
  assert(fifa96_match_run_should_mark_over(&f.mr) == 1);
  drop_fixture(&f);
}
```

- [ ] **Step 2: Run — expect fail.**
- [ ] **Step 3: Implement** — replace the proxy with the end-word read, keeping the `global_157ac2 >= 2u` half-time guard and the phase-21 `idx4 && global_157ac0` skip:
```c
/* FU-232 §4.2: the lifecycle trigger is the end word, not the derived proxy. */
int fifa96_match_run_should_mark_over(const fifa96_match_run *mr) {
  return (mr->end_word_158822 != 0u && mr->global_157ac2 >= 2u);
}
```
- [ ] **Step 4: Run — expect pass.** `ctest --test-dir build -R test_engine_match_completion --output-on-failure`
- [ ] **Step 5: Commit.**
```bash
git add src/fifa96_engine/fifa96_match_run.c tests/test_engine_match_completion.c
git commit -m "feat(engine): the mark_over tail keyed to the end word (FU-232)"
```

---

### Task 4: The match-over screen dwell + the front-end return

**Authority:** FU-235 §4 (D1: the `0x960` dwell is the `word[0x158818]` timer, the bound is `> 0x168`), FU-234 §4.2/§4.3 (the return), FU-239 §5. D1 = **model it** (a phase-`0xC` dwell on `mr->act.timer`); the `0x960`/`0x780` "camera" is a ball lead (legs L14.8.a), not a sink.

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` (the dwell gate upstream of `mark_over` `:4184`; the `run_end`/`post_exit` return `:4231`/`:1781`).
- Test: `tests/test_engine_match_frame.c` (the screen state) + `tests/test_engine_match_completion.c`.

**Interfaces:**
- Consumes: `mr->end_word_158822` (T2/T3), `mr->act.timer`.
- Produces: `mark_over` fires only after the terminal dwell (`word[0x158818] > 0x168`); `run_end` switches to `FIFA96_ENGINE_MODE_FRONTEND`.

- [ ] **Step 1: Failing tests** — (a) `mark_over` is held while the timer `<= 0x168` after the word is set, and fires past it; (b) `run_end` lands in `FIFA96_ENGINE_MODE_FRONTEND`.
- [ ] **Step 2: Run — expect fail.**
- [ ] **Step 3: Implement** — (a) the **ET-entry dwell + the live re-arm** (moved here from T1, the T1↔T4 ruling): keep the chooser writing phase `0x13` (the entry screen, preserving the v21 prefix); when the entry dwell elapses (a gate ≥ the v21 20-frame window of `state=19`; the exact native bound is a leg), call `fifa96_match_run_period_over_rearm(mr)` so the ET half enters phase `2`; then (b) wrap the `mark_over` call with the dwell gate (`mr->end_word_158822 != 0u && mr->act.timer > 0x168u`); keep `run_end`→FRONTEND (`:4241`).
- [ ] **Step 4: Run — expect pass.** `ctest --test-dir build -R 'test_engine_match_frame|test_engine_match_completion' --output-on-failure`
- [ ] **Step 5: Commit.**
```bash
git add src/fifa96_engine/fifa96_match_run.c tests/test_engine_match_frame.c
git commit -m "feat(engine): the match-over screen dwell + the front-end return (D1)"
```

---

### Task 5: The M2 tape extension v21→v22 (+ the re-pin)

**Authority:** FU-238 §2.2/§2.5, the fact sheet `t6a`/`t6b`. Additive: the v21 2522-prefix stays byte-identical; expected `N ≈ 3000–4500`.

**Files:**
- Modify: `tests/test_engine_m2.c` (the directives `:1684-1686`, the anchors `:2771-2785`, the stop `:3105-3115`, the `m2_result` `:2041-2044`, the caps `:1647-1648`, the additivity gate `:4561-4589`, the `assert(exit_step<0)` `:4349`).
- Modify: `tests/golden/engine/m2-frames.txt` (re-pin).

**Interfaces:**
- Consumes: T1–T4 (the live ET half + the terminal + the return).
- Produces: the v22 golden (a new sha) + `m2_result{et_half2_step, et_terminal_step, et_over_step, return_step}`.

- [ ] **Step 1: Extend the tape + the stop.** Add the directives `M2_M_ET_HALF2` / `M2_M_ET_TERMINAL` / `M2_M_ET_OVER` / `M2_M_RETURN` + `M2_POST_RETURN_FRAMES 20` (per FU-238 §2.2), and replace the `M2_POST_PERIOD_END_FRAMES`-past-ET-entry stop with the return-window break (t1-facts §3). Raise `M2_STEP_CAP` to the measured `N` and `M2_TRANSCRIPT_CAP` to `1u << 24`.
- [ ] **Step 2: Record + assert the anchors** — the ET half completes → the terminal (`end_word_158822 == 2u`) → the ET-over → the return (`exit_step > 0`). Flip `assert(res.exit_step < 0)` → `> 0` (`:4349`) and the additivity gate constant `792` → `2522` (`:4561-4589`).
- [ ] **Step 3: Run — expect the new transcript.** `./build/test_engine_m2 > /tmp/m2-v22.txt`; verify the prefix: `head -2522 /tmp/m2-v22.txt | cmp - tests/golden/engine/m2-frames.txt` (clean).
- [ ] **Step 4: Re-pin with a written reason.** `cp tests/golden/engine/m2-frames.txt /tmp/m2-v21.txt`; `cp /tmp/m2-v22.txt tests/golden/engine/m2-frames.txt`; record the old/new sha + line counts + the first-diff line in the plan's completion note + the commit body (the FU-238 §:287-306 template).
- [ ] **Step 5: Prove determinism + commit.**
Run: `make check` — Expected green; then
```bash
git add tests/test_engine_m2.c tests/golden/engine/m2-frames.txt
git commit -m "test(engine): extend + re-pin the M2 golden to the end spine (v21->v22)"
```

---

### Task 6: The additivity gate, the mutation-RED riders, and the errata

**Authority:** FU-238 §2.5/§3.3, the fact sheets `t6b`/`conv`.

**Files:**
- Modify: `tests/test_engine_m2.c` (`run_phase22_riders` beside `run_phase21_riders` `:4090-4250`, the provenance block `:1-1618`).
- Modify: `docs/ghidra/FU233_extra_time_halves.md`, `docs/ghidra/FU238_tape_extension_v22.md` (errata cross-refs to FU-239), `docs/PROGRESS.md`, `docs/ENGINE.md` (the slice closure).

**Interfaces:**
- Consumes: the v22 golden (T5).
- Produces: 7 mutation-RED riders + the in-test additivity gate + the errata.

- [ ] **Step 1: Add `run_phase22_riders`** with FU-238 §3.3's 7 classes: reg-over word 1; ET-over word 2; the frame-loop consumer; the ET halves live; the front-end return; the screen timing gate; the additivity/determinism gate. Each is a prose-comment break + an aborting assert (the `conv-facts` mutation-RED convention).
- [ ] **Step 2: Strengthen the in-test gate** — assert the v21 2522-prefix is byte-identical inside the producer (`:4569-4589`).
- [ ] **Step 3: Land the errata** — quoted cross-refs: FU-233 §3 / FU-238 §2.3 → FU-239 (§4: the ET half is phase `2`, `0x13`/`0x14` is the ET-over screen); FU-213 §11 → FU-239 §1 (`FUN_00088860` is the dispatcher).
- [ ] **Step 4: Update the records** — close the phase-22 slice in `docs/PROGRESS.md`; add the gates/legs to `docs/ENGINE.md`. Do **not** change the README census line (no row wired).
- [ ] **Step 5: Run + commit.**
Run: `make check` — green.
```bash
git add tests/test_engine_m2.c docs/
git commit -m "test(engine): phase-22 mutation-RED riders + the additivity gate + errata"
```

---

## Self-Review

- **Spec coverage:** §5 T1→Task 5; T2→Task 2; T3→Task 3; T4→Task 1 (revised by FU-239: the ET half is phase 2); T5→Task 4; T6→Tasks 5/6. The spec §2/§7.4 tie→shootout premise is **corrected** (FU-237: shootout OUT) — recorded in FU-239 + Task 6 errata, not implemented.
- **Placeholders:** none — each step carries the code/command; T1's route is FU239-L1 (an explicit, cited leg, not a placeholder).
- **Type consistency:** `end_word_158822` (T2) is the same cell T3/T4 read; `period_over_rearm` (T1) is the same helper T3's lifecycle consumes; `act.timer` is the `word[0x158818]` dwell used by both T2 (the ET-over gate) and T4 (the screen gate).
- **Legs to carry (never assert):** FU239-L1 (the exact `0x13→0x0C→sit-8→phase 2` route), FU239-L3 (`[0x157AC2]=4` vs the tape's forced-ET), FU-231 (the `[0x105FFC]=5` arm has no cell), FU-234-L6, FU-233-L1/L5, FU-237-L1.
- **Risk:** if T1 cannot reach phase `2` without an unprovable route (FU239-L1 fails), STOP-and-report — do not fake the clock.
