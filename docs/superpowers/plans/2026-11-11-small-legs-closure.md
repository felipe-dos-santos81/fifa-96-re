# Small-Legs Closure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the tractable remaining engine legs after the five-item closure, the replay trigger and the 23-descriptor remap pool: the A1–A4 pool legs, B1–B3 stats legs and C1–C4 replay small legs.

**Architecture:** One evidence-gated plan. The documentary legs (A1–A4, B3, C2) land first as reviewed closure records with fresh Ghidra evidence; then the code legs land one seam at a time: the scorer counter, the side-swap formula at reset, the three replay input fixes, the live-match freeze, and the audio duck. The recorded-frame ring, the 19-site `0x8ED40` body campaign and the full `FUN_0003BB1C` loop are out of scope.

**Tech Stack:** C11, CMake/CTest, SDL3, Ghidra MCP (`/FIFA96.EXE`), ASan/UBSan.

**Spec:** `docs/superpowers/specs/2026-11-11-small-legs-closure-design.md` (read it first; it carries the native evidence and the settled rules).

## Global Constraints

- Work stays on `main`; commit per task; push in Task 7 after the full gate.
- `make check` stays green (124/124), strict-clean.
- `port/tests/golden/engine/m1-frames.txt` is immutable. M1 and M2 must stay byte-exact; no golden re-pin.
- Native evidence is first-hand Ghidra on `/FIFA96.EXE` (case-sensitive). No scripts, no stale `/tmp` image. A caller/count claim needs a fresh xref in the same session.
- Anything not statically proved is a numbered leg, never an asserted table.
- No completion-number movement: no new dispatch body/row. If a change would add one, stop and recompute the README/ENGINE numbers in the same commit.
- Every task ends with `make check`; the final task adds the M1/M2 `cmp` and the windowed smoke.
- Do not modify the recorded-frame ring, the 19 unported `0x8ED40` bodies, or the `FUN_0003BB1C` UI loop.

## Review Focus

- A replay key already held when the arm fires must not produce a spurious edge (`test_replay_arm_held_key`).
- The host kickoff auto-PASS must not leak a PASS into the replay input, including on the arming step (`test_kickoff_auto_pass_replay_gated`).
- The live freeze must still run the replay step and keep the overlay visible; a frozen clock must not mean a starved replay (`test_replay_freezes_live_frame`).
- No non-zero `side_swap` ships until the native key/block mapping is proved; a flip must not attribute stats to the other team's same-numbered player (B2-L1 gate; the existing `test_stats_permuted_roster_order` stays green).
- The audio duck must not survive the replay exit and must not silence the no-bank path (`test_audio_replay_duck`).

---

### Task 1: Evidence closures for A1–A4, B3 and C2

**Files:**
- Modify: `port/spec/FU-348_substitution_remap_and_ball_legs.md` (append a small-legs closure section)
- Modify: `port/spec/FU-350_native_lineup_order.md` (append the `0x107080` writer evidence)
- Modify: `port/spec/FU72_ring_stats.md` (append the B3 census)
- Modify: `port/spec/FU-351_class3_camera.md` (append the C2 adapter closure)
- Modify: `port/ENGINE.md` (append the phase record)
- Modify: `docs/PROGRESS.md` (append the status narrative)
- Modify: `docs/superpowers/specs/2026-11-11-small-legs-closure-design.md` (append the documentary closure record)

**Interfaces:**
- Consumes: nothing.
- Produces: the closure verdicts later tasks cite: A1 dormant; A2 kit-editor preview (label correction); A3 option cells; A4 unreachable; B3 no writer; C2 host-key adapter.

- [ ] **Step 1: Re-run the fresh evidence**

Run these Ghidra MCP calls on `/FIFA96.EXE` and record the exact results:

- `get_xrefs_to 0x14BF9C` → 0.
- `get_xrefs_to 0x48DC0` → 2 (`0x56478` in `FUN_000563d0`, `0x5721A` in `FUN_00057158`).
- `get_xrefs_to 0x1068E0` → 5: reads `0x48DCE`/`0x48EE5`, writes `0x48FE1` (`FUN_00048ed8`) and `0x4902F` (`FUN_00048ff4`).
- `decompile_function 0x48DC0`, `0x48ED8`, `0x48FF4` → the preview-flag body and the set/clear pair.
- `decompile_function 0x1CAEC` → case 9 `[0x105284] == 4`, case 0xD `[0x105298] == 4`.
- `search_instructions operand_pattern 105284` and `105298` → only the CMP reads.
- `get_xrefs_to 0x107080` → 4 (three DATA reads in `FUN_00047298`, one WRITE at `0x487D9` in `FUN_000481EC`).
- `get_xrefs_to 0x481EC` → 1 (`0x48D57` in `FUN_00048d38`, which then calls `FUN_00047298(1)`).
- `disassemble_bytes 0x487A0` → `MOV EBP,0x15` and the write loop head; `disassemble_bytes 0x47740` → `CMP dword[EAX*4+0x107080],0x16`.
- `search_instructions mnemonic MOV, operand_pattern "+ 0x8d]"` → 19 loads; `"+ 0x8a]"` → 49 loads; no stores.
- `get_xrefs_to 0x64270` → 3 (`0x383B3` in `FUN_00038004`, `0x38E81` in `FUN_00038630`, `0x5112D` in `FUN_000510DC`); `decompile_function 0x510DC` → the begin arm.

Expected: every count/value matches the spec's §6. If one differs, stop and report; do not write a closure around the old value.

- [ ] **Step 2: Append the closure sections**

Use the repo's append-only errata style. Do not rewrite prior sections.

- FU-348: A1 dormant (no consumer); A2 label correction (kit-editor preview, live match unaffected); A4 unreachable (the `0x107080` lineup array); keep the four remaining legs honest.
- FU-350: the `0x107080` writer detail (`FUN_000481EC`, `0x487A0` bound 0x15, `0x48D38` calls `FUN_00047298(1)`).
- FU72: B3 fresh census (19/49 loads, no stores) and the 1.25× dead-branch verdict.
- FU-351: C2 adapter closure (the menu producer chain, the host R/BACK adapter at `FUN_00064270`, and the ring-dependent request leg).
- ENGINE: one phase paragraph with the six verdicts and the gate.
- PROGRESS: one status paragraph.
- The design doc: a `## 10. Closure record (documentary legs)` section with the fresh commands and the verdicts.

- [ ] **Step 3: Run the gate**

Run: `make check`
Expected: 124/124, strict-clean (no code changed).

Run: `./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt`
Run: `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`
Expected: both exit 0.

- [ ] **Step 4: Commit**

```bash
git add port/spec/FU-348_substitution_remap_and_ball_legs.md port/spec/FU-350_native_lineup_order.md port/spec/FU72_ring_stats.md port/spec/FU-351_class3_camera.md port/ENGINE.md docs/PROGRESS.md docs/superpowers/specs/2026-11-11-small-legs-closure-design.md
git commit -m "docs: close the documentary small legs"
```

---

### Task 2: B1 — the `FUN_0008ECC4` scorer counter

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (the `stats_third_selection` cell and the bump declaration)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (the bump and the score-event call)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: `struct fifa96_match_stats` (`key/count/value`), `fifa96_match_run_stats_seed`, `match_run_replay_roster_id`, `match_run_replay_note_goal`.
- Produces:
  - `uint8_t stats_third_selection;` in `struct fifa96_match_run`, next to `stats`.
  - `int fifa96_match_run_stats_bump(struct fifa96_match_run *mr, int32_t entity_id);` — `1` on a hit, `0` on NULL/out-of-range/no matching key. Resolves the block with `(team[side].side ^ side_swap) & 1`, scans that block's 16 slots for the key `match_run_replay_roster_id(side, slot)`, increments the `uint16_t count`, and sets `stats_third_selection = (count == 3)`.

- [ ] **Step 1: Write the failing test**

Add `test_stats_counter_bump` to `port/tests/test_engine_match_handlers.c` and register it in `main()`:

```c
static void test_stats_counter_bump(void) {
  struct fixture f;
  make_fixture(&f);
  for (uint32_t side = 0; side < 2u; side++) {
    for (uint32_t slot = 0; slot < FIFA96_PCCD_ROSTER_COUNT; slot++)
      f.mr.roster_order[side][slot] = (uint8_t)slot;
    f.mr.roster_order_ok[side] = 1;
  }
  fifa96_match_run_stats_seed(&f.mr);
  assert(fifa96_match_run_stats_bump(NULL, 0) == 0);
  assert(fifa96_match_run_stats_bump(&f.mr, 0x99) == 0);
  assert(fifa96_match_run_stats_bump(&f.mr, 0 * 11 + 3) == 1);
  assert(f.mr.stats.count[0][3] == 1 && f.mr.stats_third_selection == 0);
  assert(fifa96_match_run_stats_bump(&f.mr, 0 * 11 + 3) == 1);
  assert(f.mr.stats.count[0][3] == 2 && f.mr.stats_third_selection == 0);
  assert(fifa96_match_run_stats_bump(&f.mr, 0 * 11 + 3) == 1);
  assert(f.mr.stats.count[0][3] == 3 && f.mr.stats_third_selection == 1);
  assert(fifa96_match_run_stats_bump(&f.mr, 0 * 11 + 3) == 1);
  assert(f.mr.stats.count[0][3] == 4 && f.mr.stats_third_selection == 0);
  fifa96_match_run_stats_seed(&f.mr);
  assert(f.mr.stats.count[0][3] == 0 && f.mr.stats_third_selection == 0);
  drop_fixture(&f);
}
```

Add `test_stats_counter_score_event` to the same file:

```c
static void test_stats_counter_score_event(void) {
  struct fixture f;
  make_fixture(&f);
  for (uint32_t slot = 0; slot < FIFA96_PCCD_ROSTER_COUNT; slot++)
    f.mr.roster_order[0][slot] = (uint8_t)slot;
  f.mr.roster_order_ok[0] = 1;
  fifa96_match_run_stats_seed(&f.mr);
  f.mr.goal_record = 0 * 11 + 3;
  assert(fifa96_match_run_score_event(&f.mr, 0, 0) == 0);
  assert(f.mr.stats.count[0][3] == 1);
  drop_fixture(&f);
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: compile error / FAIL because `fifa96_match_run_stats_bump` and `stats_third_selection` do not exist.

- [ ] **Step 3: Implement the bump and the score-event call**

Add the declaration and the cell to `fifa96_match_run.h`. Implement `fifa96_match_run_stats_bump` in `fifa96_match_run.c` next to `fifa96_match_run_stats_add`, using the same block/key resolution. Call it from `match_run_replay_note_goal` when `mr->goal_record >= 0` (the native `FUN_0008A938` case 6 `[0x157A9B] == NULL` branch; the engine has no `[0x157A9B]` producer, so the scorer path is the live branch).

The native body is `FUN_0008ECC4` (fresh decompile): it scans the selected block's 16 slots, increments the `+4` word, and sets `[0x5B358] = (count == 3)` (zero otherwise). Do not touch `activity_count`; that is the `0x8ED40` path.

- [ ] **Step 4: Run the tests to verify they pass**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 5: Run the full gate**

Run: `make check`
Expected: 124/124, strict-clean.

- [ ] **Step 6: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): bump the scorer activity counter"
```

---

### Task 3: B2 — the `side_swap` formula and the reset seam

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (the pure helper declaration)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (the helper and the two reset assignments)
- Test: `port/tests/test_engine_match_handlers.c`
- Modify: `port/spec/FU72_ring_stats.md` and `port/ENGINE.md` (the B2-L1 leg record)

**Interfaces:**
- Consumes: `mr->global_157ac2`, `mr->side_swap`.
- Produces: `uint8_t fifa96_match_side_swap_value(uint8_t native_157ac2, uint8_t native_157ac3);` — `(native_157ac2 ^ native_157ac3) & 1u`. The run's reset calls it with `(mr->global_157ac2, 0u)`; at reset `global_157ac2` is 0, so the value stays 0. **No non-zero flip is wired.**

- [ ] **Step 1: Write the failing test**

Add `test_side_swap_value` to `port/tests/test_engine_match_handlers.c` and register it in `main()`:

```c
static void test_side_swap_value(void) {
  assert(fifa96_match_side_swap_value(0, 0) == 0);
  assert(fifa96_match_side_swap_value(1, 0) == 1);
  assert(fifa96_match_side_swap_value(2, 0) == 0);
  assert(fifa96_match_side_swap_value(3, 0) == 1);
  assert(fifa96_match_side_swap_value(0, 1) == 1);
  assert(fifa96_match_side_swap_value(0xFF, 0) == 1);
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: compile error / FAIL because the helper does not exist.

- [ ] **Step 3: Implement the helper and replace the reset assignments**

Add the pure helper to `fifa96_match_run.h`/`.c`. Replace both `mr->side_swap = 0;` assignments with `mr->side_swap = fifa96_match_side_swap_value(mr->global_157ac2, 0u);`. Record B2-L1 in FU72 and ENGINE: the native `[0x157AC2]` is a phase/mode byte (56 fresh refs, writers at `0x4B0DE`/`0x4BA20`/`0x4BA31`/`0x38990`/`0x73F82`/`0x88D6A`/`0x8B58A`/`0x8B61B`); the `0x742B4` writer's caller is not created; the non-zero flip and the stats-key mapping are not proved, so the flip stays a leg.

- [ ] **Step 4: Run the test to verify it passes**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 5: Run the full gate**

Run: `make check`
Expected: 124/124; M1/M2 unchanged (the value stays 0).

- [ ] **Step 6: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_handlers.c port/spec/FU72_ring_stats.md port/ENGINE.md
git commit -m "feat(engine): model the native side-swap formula at reset"
```

---

### Task 4: C1 — the three replay input minors

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (`input_primed` and `input_resync` cells)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`replay_begin`/`replay_exit`/input)
- Modify: `port/src/fifa96_engine/fifa96_engine.c` (the kickoff auto-PASS/dwell gate)
- Test: `port/tests/test_engine_match_render.c` (held-key and switch/tackle)
- Test: `port/tests/test_engine_match_bridge.c` (auto-PASS)

**Interfaces:**
- Consumes: `fifa96_engine_key` codes, `fifa96_match_run_replay_family`, `mr->live_switch_press`/`live_tackle_press`/`live_switch_down`/`live_tackle_down`.
- Produces: `uint8_t input_primed;` in `struct fifa96_match_run_replay`; `uint8_t input_resync;` in `struct fifa96_match_run`. `replay_begin` sets `input_primed = 0`, `input_resync = 0`, and clears the two press flags; `replay_exit` sets `input_resync = 1`.

- [ ] **Step 1: Write the failing tests**

Add to `port/tests/test_engine_match_render.c` and register both in `main()`:

```c
static void test_replay_arm_held_key(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.lc.screen = FIFA96_MATCH_SCREEN_ACTIVE;
  assert(fifa96_match_run_replay_begin(&mr) == 0);
  fifa96_platform_key held = {FIFA96_ENGINE_KEY_PASS, 1};
  assert(fifa96_match_run_input(&mr, &held, 1) == 0);
  assert(mr.render.replay.input_buttons == 0);   /* no edge from a held key */
  assert(mr.render.replay.input_buttons_held == 0x20u);
  fifa96_platform_key none = {0, 0};
  assert(fifa96_match_run_input(&mr, &none, 1) == 0);
  assert(fifa96_match_run_input(&mr, &held, 1) == 0);
  assert(mr.render.replay.input_buttons == 0x20u); /* a fresh press does edge */
  fifa96_match_run_init(&mr);
}

static void test_replay_switch_tackle_resync(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.lc.screen = FIFA96_MATCH_SCREEN_ACTIVE;
  mr.live_start = 1;
  mr.state.phase = 2;                 /* the switch_accept phase gate */
  mr.live_switch_press = 1;
  mr.live_tackle_press = 1;
  assert(fifa96_match_run_replay_begin(&mr) == 0);
  assert(mr.live_switch_press == 0 && mr.live_tackle_press == 0);
  fifa96_match_run_replay_exit(&mr);
  fifa96_platform_key none = {0, 0};
  assert(fifa96_match_run_input(&mr, &none, 1) == 0);
  assert(mr.live_switch_press == 0 && mr.live_tackle_press == 0);
  fifa96_platform_key sw = {FIFA96_ENGINE_KEY_SWITCH, 1};
  assert(fifa96_match_run_input(&mr, &sw, 1) == 0);
  assert(mr.live_switch_press == 1);
  fifa96_match_run_init(&mr);
}
```

Add to `port/tests/test_engine_match_bridge.c` and register it in `main()`:

```c
static void test_kickoff_auto_pass_replay_gated(void) {
  const fifa96_platform_key tape[] = {
      {FIFA96_ENGINE_KEY_DECLINE, 1}, {FIFA96_ENGINE_KEY_DECLINE, 0},
      {FIFA96_ENGINE_KEY_CONFIRM, 1}, {FIFA96_ENGINE_KEY_CONFIRM, 0},
      {FIFA96_ENGINE_KEY_REPLAY, 1},
  };
  struct fixture f = make_fixture(tape, sizeof tape / sizeof tape[0], NULL);
  assert(fifa96_engine_step(f.engine) == 0);   /* DECLINE -> panel */
  assert(fifa96_engine_step(f.engine) == 0);   /* release */
  assert(fifa96_engine_step(f.engine) == 0);   /* CONFIRM -> bridge */
  assert(fifa96_engine_step(f.engine) == 0);   /* CONFIRM release */
  assert(f.engine->match != NULL);
  f.engine->match->state.phase = 1;            /* the kickoff wait */
  f.engine->match->global_157ac2 = 0;
  f.engine->match_halftime_steps = 300u;
  f.engine->match_halftime_pressed = 1u;
  f.engine->match_halftime_hold = 10u;
  assert(fifa96_engine_step(f.engine) == 0);   /* R: arm on this step */
  assert(fifa96_match_run_replay_family(&f.engine->match_run.render.replay));
  assert(f.engine->match_halftime_hold == 10u); /* the dwell paused */
  drop_fixture(f);
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cmake --build build --target test_engine_match_render test_engine_match_bridge -j"$(nproc)" && ./build/test_engine_match_render && ./build/test_engine_match_bridge`
Expected: `test_replay_arm_held_key` fails (held PASS produces `input_buttons == 0x20` today); the auto-PASS test fails (the dwell decrements on the arming step).

- [ ] **Step 3: Implement the three fixes**

1. Replay input priming: in the replay branch of `fifa96_match_run_input`, if `!r->input_primed`, set `r->input_buttons_held = held`, `r->input_buttons = 0`, `r->input_primed = 1`, and return. Otherwise keep the existing edge accumulation.
2. Switch/tackle: `replay_begin` clears `live_switch_press`/`live_tackle_press`; `replay_exit` sets `input_resync = 1`. In the non-replay input path, if `mr->input_resync`, set the two down latches to the current `switch_down`/`tackle_down`, clear the two press flags, clear `input_resync`, and skip the edge branches for this poll (still fold the FU-61 word).
3. Auto-PASS: in `fifa96_engine.c`, **move the replay-down/edge computation
   above the kickoff block** (the current `replay_down`/`replay_edge`/
   `match_replay_held` lines are below it), then compute
   `replay_guard = e->match && (fifa96_match_run_replay_family(&e->match->render.replay) || replay_edge);`
   and wrap the whole kickoff wait block — the auto-PASS injection and the
   `match_halftime_*` dwell updates — in `if (!replay_guard) { ... }`. The
   later arm uses the same variables. The dwell therefore freezes for the
   arming step and every replay frame, and resumes on exit.

- [ ] **Step 4: Run the tests to verify they pass**

Run: `cmake --build build --target test_engine_match_render test_engine_match_bridge -j"$(nproc)" && ./build/test_engine_match_render && ./build/test_engine_match_bridge`
Expected: PASS.

- [ ] **Step 5: Run the full gate**

Run: `make check`
Expected: 124/124, strict-clean.

- [ ] **Step 6: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_engine.c port/tests/test_engine_match_render.c port/tests/test_engine_match_bridge.c
git commit -m "fix(engine): freeze the replay input edges and the kickoff auto-pass"
```

---

### Task 5: C3 — the live-match freeze

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`fifa96_match_run_frame`)
- Test: `port/tests/test_engine_match_frame.c`

**Interfaces:**
- Consumes: `fifa96_match_run_replay_family`, `fifa96_match_pace_tick`, `FIFA96_MATCH_STATE_STEP`, `fifa96_match_run_replay_step`.
- Produces: the frozen-frame behavior; no new public API. While the replay family is active, the pace grant is consumed directly, `state.frame_delta = FIFA96_MATCH_STATE_STEP >> 8`, the replay step runs, and the live simulation/clock/kickoff timers do not advance.

- [ ] **Step 1: Write the failing test**

Add `test_replay_freezes_live_frame` to `port/tests/test_engine_match_frame.c` and register it in `main()`. Use the file's existing run helper pattern; the grant loop is deterministic (the pace grants 3 frames per 10 ticks):

```c
static int step_until_granted(struct fifa96_match_run *mr) {
  for (int i = 0; i < 16; i++) {
    int rc = fifa96_match_run_frame(mr);
    assert(rc >= 0);
    if (rc == 1) return 1;
  }
  return 0;
}

static void test_replay_freezes_live_frame(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.lc.screen = FIFA96_MATCH_SCREEN_ACTIVE;
  mr.state.phase = 2;                  /* clock class 1 */
  mr.state.period_length = 90;
  mr.state.period_seconds = 30;
  mr.state.total_seconds = 30;
  mr.state.tick_total = 0x1234;
  mr.entities.team[0].records[0].pos_x = 77;
  assert(fifa96_match_run_replay_begin(&mr) == 0);
  uint16_t seconds = mr.state.period_seconds;
  uint16_t total = mr.state.total_seconds;
  uint16_t ticks = mr.state.tick_total;
  int16_t x = mr.entities.team[0].records[0].pos_x;
  for (int i = 0; i < 3; i++) assert(step_until_granted(&mr));
  assert(mr.state.period_seconds == seconds);
  assert(mr.state.total_seconds == total);
  assert(mr.state.tick_total == ticks);
  assert(mr.entities.team[0].records[0].pos_x == x);
  assert(mr.render.replay.state == FIFA96_MATCH_REPLAY_PLAY);
  assert(mr.state.frame_delta != 0);
  fifa96_match_run_replay_exit(&mr);
  assert(step_until_granted(&mr));
  assert(mr.state.tick_total != ticks);   /* the clock resumed */
  fifa96_match_run_init(&mr);
}
```

If the file's existing `one_granted_frame` helper already does this, use it instead; the assertion set is the requirement.

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_frame -j"$(nproc)" && ./build/test_engine_match_frame`
Expected: FAIL — the clock/ticks/position advance during replay today.

- [ ] **Step 3: Implement the freeze**

At the top of `fifa96_match_run_frame`, branch on `fifa96_match_run_replay_family(&mr->render.replay)`:

- Frozen path: call `fifa96_match_pace_tick(&mr->pace, 0, &granted)`; if `granted == 0`, return 0. Set `mr->state.frame_delta = (uint16_t)(FIFA96_MATCH_STATE_STEP >> 8)`. For each granted frame, run `fifa96_match_run_replay_step(mr, mr->render.replay.input_buttons, (int32_t)mr->state.frame_delta)` and clear `input_buttons` (the existing seam).
- Skip the live simulation: the kickoff timers, the live input model, the control-slot update, the entity/ball update and the clock tick. Keep the common presentation/camera/render tail outside the guard.
- Normal path: unchanged `fifa96_match_state_tick` and live body.

The exact section split is the implementer's read of the function; the test pins the behavior. Do not skip the replay step or the overlay.

- [ ] **Step 4: Run the test to verify it passes**

Run: `cmake --build build --target test_engine_match_frame -j"$(nproc)" && ./build/test_engine_match_frame`
Expected: PASS.

- [ ] **Step 5: Run the full gate**

Run: `make check`
Expected: 124/124; M1/M2 byte-exact (the tapes never enter the family).

- [ ] **Step 6: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_frame.c
git commit -m "feat(engine): freeze the live match while replay is active"
```

---

### Task 6: C4 — the pre-arm audio duck

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_audio.h` (the duck flag and the two hooks)
- Modify: `port/src/fifa96_engine/fifa96_match_audio.c` (the duck, the crowd stop/re-arm gate)
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (`replay_audio_armed` cell)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (set/clear the cell in `replay_begin`/`replay_exit`)
- Modify: `port/src/fifa96_engine/fifa96_engine.c` (the arm/release edge at the audio step)
- Test: `port/tests/test_engine_audio.c`

**Interfaces:**
- Consumes: `struct fifa96_match_audio`, `fifa96_mixer_stop`, `fifa96_mixer_voice_active`, `match_audio_scale_volume`, `match_audio_arm_crowd`.
- Produces:
  - `uint8_t replay_ducked;` in `struct fifa96_match_audio`.
  - `void fifa96_match_audio_replay_arm(struct fifa96_match_audio *a);` — idempotent; stops `crowd_voice` (`fifa96_mixer_stop`, set `crowd_voice = -1`), sets `replay_ducked = 1`.
  - `void fifa96_match_audio_replay_release(struct fifa96_match_audio *a);` — clears `replay_ducked`; the next `audio_step` re-arms the crowd.
  - `uint8_t replay_audio_armed;` in `struct fifa96_match_run`; `replay_begin` sets 1, `replay_exit` sets 0.

- [ ] **Step 1: Write the failing test**

Add the no-bank arm/release assertions to `test_no_assets_is_silent` (after the current `a.active == 1` checks), and add the retail-bank duck block inside `test_retail_banks` after the crowd-loop assertion (`fifa96_mixer_voice_active(&a.mixer, a.crowd_voice) == 1`):

```c
  /* C4: the pre-arm duck is persistent and idempotent. */
  int voice = a.crowd_voice;
  fifa96_match_audio_replay_arm(&a);
  assert(a.replay_ducked == 1);
  assert(!fifa96_mixer_voice_active(&a.mixer, voice));
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.crowd_voice == -1);                 /* no re-arm while ducked */
  fifa96_match_audio_replay_arm(&a);           /* idempotent */
  assert(a.replay_ducked == 1 && a.crowd_voice == -1);
  fifa96_match_audio_replay_release(&a);
  assert(a.replay_ducked == 0);
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.crowd_voice >= 0);
  assert(fifa96_mixer_voice_active(&a.mixer, a.crowd_voice));
```

The no-bank block:

```c
  fifa96_match_audio_replay_arm(&a);
  assert(a.replay_ducked == 1 && a.crowd_voice == -1);
  fifa96_match_audio_replay_release(&a);
  assert(a.replay_ducked == 0);
```

Also extend `test_engine_replay_key_arms` in `port/tests/test_engine_match_bridge.c`: after the R arm assert `f.engine->match_run.replay_audio_armed == 1`; after the exit loop assert `== 0`. The audio hook edge itself is covered by the direct retail test.

The retail half stays ISO-gated exactly like the existing test.

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_audio -j"$(nproc)" && ./build/test_engine_audio`
Expected: compile error / FAIL because the hooks and the flag do not exist.

- [ ] **Step 3: Implement the duck**

In `fifa96_match_audio.c`, add the flag and the two hooks. In `match_audio_scale_volume` callers, use `a->replay_ducked ? a->sfx_percent / 3u : a->sfx_percent` (a small `match_audio_percent(a)` helper). Guard the crowd re-arm in `fifa96_match_audio_step` with `!a->replay_ducked`. In `replay_begin`/`replay_exit`, set/clear `replay_audio_armed`. At the `fifa96_match_audio_step` call site in `fifa96_engine.c`, call `replay_arm`/`replay_release` on the run-cell edge (`armed && !ducked` / `!armed && ducked`). The hook is silent with no banks.

- [ ] **Step 4: Run the test to verify it passes**

Run: `cmake --build build --target test_engine_audio -j"$(nproc)" && ./build/test_engine_audio`
Expected: PASS (or the asset-gated skip when the fixture has no bank).

- [ ] **Step 5: Run the full gate**

Run: `make check`
Expected: 124/124, strict-clean.

- [ ] **Step 6: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_audio.h port/src/fifa96_engine/fifa96_match_audio.c port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_engine.c port/tests/test_engine_audio.c
git commit -m "feat(audio): duck the match audio while replay is armed"
```

---

### Task 7: Closure records, final gate and push

**Files:**
- Modify: `docs/superpowers/specs/2026-11-11-small-legs-closure-design.md` (append the code-leg closure record)
- Modify: `port/spec/FU72_ring_stats.md` (B1/B2 closure)
- Modify: `port/spec/FU-351_class3_camera.md` (C1/C3/C4 closure)
- Modify: `port/ENGINE.md`, `docs/PROGRESS.md` (final status)
- Test: the full gate

**Interfaces:**
- Consumes: Tasks 1–6.
- Produces: the recorded closure and the remaining legs.

- [ ] **Step 1: Append the closure records**

Record, per leg: B1 counter (tests, the `[0x5B358]` consumer `0x8FF30` leg), B2 formula (the reset seam, B2-L1 flip leg), C1 input (the three tests), C3 freeze (the test, the pace-grant rule), C4 audio (the duck, C4-L1 de-arm leg). Keep the remaining out-of-scope items named: the ring, the 19-site body campaign, the UI loop, the 23rd-slot consumer, the `FUN_0001CAEC` option UI, the `0x107080` special branch (closed unreachable), and the `[0x157AC2]` flip.

- [ ] **Step 2: Run the final gate**

Run: `make check`
Expected: 124/124, 100% passed, strict-clean.

Run: `./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt`
Run: `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`
Expected: both exit 0.

Run: `timeout 20s env DISPLAY=:1 make game`
Expected: exit 124 with a clean log.

- [ ] **Step 3: Commit and push**

```bash
git add docs/superpowers/specs/2026-11-11-small-legs-closure-design.md port/spec/FU72_ring_stats.md port/spec/FU-351_class3_camera.md port/ENGINE.md docs/PROGRESS.md
git commit -m "docs: close the small-legs campaign"
git push origin main
git status --short --branch
```

Expected: `main == origin/main`.

---

## Self-Review

- **Spec coverage:** A1/A2/A3/A4/B3/C2 → Task 1; B1 → Task 2; B2 → Task 3; C1 → Task 4; C3 → Task 5; C4 → Task 6; closure/gate → Task 7.
- **Step scan:** every code task has a failing test, a RED command, an implementation, a GREEN command, the full gate and a commit. The docs task has fresh evidence commands and the gate.
- **Type consistency:** `fifa96_match_run_stats_bump`, `fifa96_match_side_swap_value`, `fifa96_match_audio_replay_arm`/`_release`, `replay_audio_armed`, `stats_third_selection`, `input_primed`, `input_resync` are used with the same names in every task.
- **Review Focus:** each line has a test in its owning task; the B2 flip is guarded by B2-L1 and the existing permuted-order test.
- **Proportion:** the plan keeps code bodies out except the pure-byte tests and the exact assertions; the implementer writes the bodies from the signatures and the native facts.
