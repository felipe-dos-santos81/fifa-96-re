# `FUN_0008ED40` Stats Wiring Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the native 2×16 activity table to the run and wire the ported `FUN_0008ED40` call sites with the native 2x weight, so replay source-4 sees real activity counts.

**Architecture:** A run-level `fifa96_match_stats` table keyed by the same side/roster id the replay target builds. `fifa96_match_stats_add` applies the native weight (`2x` in the modeled match, because `+0x8D` has no writer and stays 0), accumulates the dword value and publishes the max triple. The engine wrapper mirrors the value into `render.replay.activity_count` so the existing selector consumes it unchanged. Wire the ported row-1B and row-09/0B sites; the row-1C, camera/announce and other unported-body sites stay numbered legs.

**Tech Stack:** C11, CMake/CTest, ASan/UBSan, Ghidra MCP evidence on `/FIFA96.EXE`.

**Spec:** `docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md` (Slice 3, §8) and `port/spec/FU72_ring_stats.md` (erratum).

## Global Constraints

- `make check` stays green: strict build (`-Wall -Wextra -Werror`) + full CTest (currently 124/124).
- M1/M2 stay byte-exact; any golden movement is a stop, not a re-pin.
- The native weight base is `[[entity]+0x7AE]` (formation layout `0x11033A + id*0x1D`); the modeled `+0x8D` is 0, so the live weight is `2x` and the 1.25x branch is a numbered leg.
- The table is 2 blocks × 16 slots, stride `0xA`: key `[[entity+4]]`, word counter `+4`, dword value `+6`; the engine keys by the replay roster id.
- Wire only the sites whose enclosing body is ported; record the rest as legs, do not invent calls.
- Work on `main`; commit per task and push.

## Review Focus

1. **Key mismatch** — a stats key that disagrees with `match_run_replay_roster_id` means source-4 never sees the count. Pinned by Task 1/4 tests.
2. **Wrong weight** — the native 2x branch must be used; the 1.25x branch stays a leg until a `+0x8D` writer is found. Pinned by Task 1.
3. **Double count** — the row-1B tracked call is a second `8ED40(tracked, 5)`, not a duplicate of the self call. Pinned by Task 2.
4. **Table reset** — a reused run must not inherit counts; init/begin/end clear the table and keys. Pinned by Task 1.
5. **M2 movement** — the accumulator writes no draw/clock cell; any M2 movement is a stop. Pinned by Task 4.

---

### Task 1: Stats table and weighted add

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (struct and declarations)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (reset/init/add/wrapper, init/begin/end wiring)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: `fifa96_stats_weight` (`fifa96_ring_stats.h`), `match_run_replay_roster_id` (static in `fifa96_match_run.c`).
- Produces:
  - `#define FIFA96_MATCH_STATS_SLOTS 16u`
  - `struct fifa96_match_stats { int32_t key[2][16]; uint16_t count[2][16]; int32_t value[2][16]; int32_t max_value; int32_t max_key; uint16_t max_block; };`
  - `void fifa96_match_stats_reset(struct fifa96_match_stats *s);`
  - `int fifa96_match_stats_add(struct fifa96_match_stats *s, uint32_t block, int32_t key, int32_t delta);`
  - `int fifa96_match_run_stats_add(struct fifa96_match_run *mr, int32_t entity_id, int32_t delta);`

- [ ] **Step 1: Write the failing table test**

Add to `port/tests/test_engine_match_handlers.c`:

```c
static void test_stats_table_add(void) {
  struct fifa96_match_stats s;
  fifa96_match_stats_reset(&s);
  s.key[0][3] = 7;
  assert(fifa96_match_stats_add(&s, 0, 7, 100) == 1);
  assert(s.value[0][3] == 200);          /* the native 2x weight */
  assert(s.max_value == 200 && s.max_key == 7 && s.max_block == 0);
  assert(fifa96_match_stats_add(&s, 0, 7, -25) == 1);
  assert(s.value[0][3] == 150);          /* -25 * 2 */
  assert(fifa96_match_stats_add(&s, 0, 0x99, 5) == 0);
  assert(fifa96_match_stats_add(NULL, 0, 7, 1) == 0);
  fifa96_match_stats_reset(&s);
  assert(s.value[0][3] == 0 && s.key[0][3] == FIFA96_MATCH_ENTITY_NONE);
}
```

Call it from `main`.

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL — unknown `struct fifa96_match_stats` / `fifa96_match_stats_reset`.

- [ ] **Step 3: Add the table, reset and add**

In `fifa96_match_run.h`, near the run struct:

```c
#define FIFA96_MATCH_STATS_SLOTS 16u
struct fifa96_match_stats {
  int32_t key[2][FIFA96_MATCH_STATS_SLOTS];
  uint16_t count[2][FIFA96_MATCH_STATS_SLOTS];
  int32_t value[2][FIFA96_MATCH_STATS_SLOTS];
  int32_t max_value;
  int32_t max_key;
  uint16_t max_block;
};
```

Add `struct fifa96_match_stats stats;` to `struct fifa96_match_run`, and declare the four functions above. Add `#include "fifa96_loader/fifa96_ring_stats.h"` to `fifa96_match_run.c` (it currently does not include it).

In `fifa96_match_run.c`:

```c
void fifa96_match_stats_reset(struct fifa96_match_stats *s) {
  if (!s) return;
  for (uint32_t side = 0; side < 2u; side++)
    for (uint32_t slot = 0; slot < FIFA96_MATCH_STATS_SLOTS; slot++) {
      s->key[side][slot] = FIFA96_MATCH_ENTITY_NONE;
      s->count[side][slot] = 0;
      s->value[side][slot] = 0;
    }
  s->max_value = 0;
  s->max_key = FIFA96_MATCH_ENTITY_NONE;
  s->max_block = 0;
}

int fifa96_match_stats_add(struct fifa96_match_stats *s, uint32_t block,
                           int32_t key, int32_t delta) {
  if (!s || block > 1u) return 0;
  for (uint32_t slot = 0; slot < FIFA96_MATCH_STATS_SLOTS; slot++) {
    if (s->key[block][slot] != key) continue;
    s->value[block][slot] += fifa96_stats_weight(delta, 0, 0);
    {
      /* The native max compares the sign-extended low word (`0x8EDC2`). */
      int16_t v = (int16_t)s->value[block][slot];
      if (v > (int16_t)s->max_value) {
        s->max_value = v;
        s->max_key = key;
        s->max_block = (uint16_t)block;
      }
    }
    return 1;
  }
  return 0;
}
```

`fifa96_stats_weight(delta, 0, 0)` returns `delta * 2` (the native `AL <= AH` arm).

- [ ] **Step 4: Add the engine wrapper and key init**

```c
static void match_run_stats_seed(struct fifa96_match_run *mr) {
  if (!mr) return;
  fifa96_match_stats_reset(&mr->stats);
  for (uint32_t side = 0; side < 2u; side++)
    for (uint32_t slot = 0; slot < FIFA96_MATCH_STATS_SLOTS; slot++)
      mr->stats.key[side][slot] = match_run_replay_roster_id(mr, side, slot);
}

int fifa96_match_run_stats_add(struct fifa96_match_run *mr, int32_t entity_id,
                               int32_t delta) {
  uint32_t side, pitch, slot;
  if (!mr || entity_id < 0 ||
      entity_id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  side = (uint32_t)entity_id / FIFA96_MATCH_ENTITY_RECORDS;
  pitch = (uint32_t)entity_id % FIFA96_MATCH_ENTITY_RECORDS;
  slot = pitch;
  if (mr->roster_order_ok[side] != 0) {
    for (uint32_t i = 0; i < FIFA96_MATCH_STATS_SLOTS; i++)
      if (mr->stats.key[side][i] ==
          match_run_replay_roster_id(mr, side, pitch)) { slot = i; break; }
  }
  if (fifa96_match_stats_add(&mr->stats, side, mr->stats.key[side][slot],
                             delta) == 0)
    return 0;
  if (slot < FIFA96_CAMERA_REPLAY_SLOTS)
    mr->render.replay.activity_count[side][slot] = mr->stats.value[side][slot];
  return 1;
}
```

Call `match_run_stats_seed(mr)` after the roster order is staged in `fifa96_match_run_begin` and in the act-9 reset; call `fifa96_match_stats_reset(&mr->stats)` in `fifa96_match_run_init`/`end` so a reused run starts clean.

- [ ] **Step 5: Run the test**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): add the native activity stats table"
```

---

### Task 2: Wire the row-1B sites

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c` (`match_1b_deep_pan` stat block)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: `fifa96_match_run_stats_add` from Task 1.
- Produces: the row-1B self/tracked activity values.

- [ ] **Step 1: Write the failing row-1B stats assertions**

In `test_action_1b_wide_route_binds_pan` (`port/tests/test_engine_match_handlers.c:5520`), after the dispatch, add an assertion on `f.mr.stats.value[0][<key>]` for the record's side/key. Use the fixture's `stat_id`/`stat_tracked` branch; if the fixture's branch is not deterministic, add a focused `test_action_1b_stats` with the same staging and assert the value is non-zero for the self key.

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL — the stats value is still 0.

- [ ] **Step 3: Add the two calls**

In the `match1b_stat_id` block, after `stat_id`/`stat_tracked` are computed and before/after `mr->match1b_stat_id = stat_id;`:

```c
if (stat_id != 0xFFu)
  (void)fifa96_match_run_stats_add(mr, r->entity_id, (int32_t)stat_id);
if (stat_tracked != 0u && cam->tracked != FIFA96_CAMERA_TRACKED_NONE)
  (void)fifa96_match_run_stats_add(mr, cam->tracked, 5);
```

The self call covers `0x77451` (+0x64), `0x7749F` (+0x19) and `0x774B2`/`0x77613` (+5); the tracked call covers `0x77493`/`0x77613` (+5).

- [ ] **Step 4: Run the test**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_handlers.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): accumulate row-1B activity"
```

---

### Task 3: Wire the row-09/0B sites

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c` (row 09 arm 1 and row 0B arm 1)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: `fifa96_match_run_stats_add` from Task 1.
- Produces: the row-09/0B `+2` / `-3` activity values.

- [ ] **Step 1: Write the failing row-09/0B stats assertions**

Extend `test_action_09_arm1_installs` and `test_action_0b_duel_chain_installs_row_5e` with a `stats.value` assertion for the acting record's key. The native deltas are `+2` (`0x80D6C`/`0x80D99`/`0x813CD`/`0x813F5`) and `-3` (`0x81BBE`).

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL — the stats value is still 0.

- [ ] **Step 3: Add the calls**

In `fifa96_match_action_09`:

- in the `ctl->anim_id == 0x4A` branch that sets `match09_event_id = 0x67`
  (`0x80D50..0x80D7C`), call
  `(void)fifa96_match_run_stats_add(mr, controlled, 2);`
- in the else branch that sets `match09_event_id = 0x5D`
  (`0x80D8D..0x80DA5`), call
  `(void)fifa96_match_run_stats_add(mr, r->entity_id, 2);`
- the target-block pair `0x813CD`/`0x813F5` is the same `+2`; wire it only if
  the engine’s target block carries the matching `0x813A2` branch, otherwise
  record the pair as a leg.

In `fifa96_match_action_0B`, before the `0x81BB7..0x81BBE` impact tail, call
`(void)fifa96_match_run_stats_add(mr, r->entity_id, -3);`.

- [ ] **Step 4: Run the test**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_handlers.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): accumulate row-09/0B activity"
```

---

### Task 4: Replay consumption, docs and gate

**Files:**
- Test: `port/tests/test_engine_match_render.c` (replay target sees the stats value)
- Modify: `port/spec/FU72_ring_stats.md` (closure/remaining sites)
- Modify: `port/spec/FU-351_class3_camera.md` (§23 activity leg)
- Modify: `port/ENGINE.md`, `docs/PROGRESS.md`
- Modify: `docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md` (§8 closure)

**Interfaces:**
- Consumes: Tasks 1–3.
- Produces: the recorded closure and the remaining-site list.

- [ ] **Step 1: Add the replay consumption test**

In `test_engine_match_render.c`, stage a non-zero activity via
`fifa96_match_run_stats_add` (not a direct struct write, so the mirror is
exercised) with a key matching `match_run_replay_roster_id`, then assert
`fifa96_match_run_replay_target` selects from it. The mirror in
`render.replay.activity_count` must equal the stats value.

- [ ] **Step 2: Update the records**

- FU-72: record the table/weight/wired sites and the remaining `8ED40` sites in unported bodies (the camera `0x70DE0`/`0x7131C`/`0x72AC4`/`0x72478` family, the row-1C branches, the `0x8AD54` announce and any site whose enclosing body is not ported).
- FU-351 §23: replace “non-goal `FUN_0008ED40` updates remain” with the wired count and the remaining sites.
- ENGINE and PROGRESS: phase 115 with the table, the 2x weight, the wired sites, the remaining legs and the gate.
- Design §8: mark the slice closed or partially closed with the remaining legs.

- [ ] **Step 3: Run the full gate**

Run: `make check` -> 124/124.
Run M1/M2 `cmp` -> exact.
Run the windowed smoke -> exit 124 with a clean log.

- [ ] **Step 4: Commit and push**

```bash
git add port/spec/FU72_ring_stats.md port/spec/FU-351_class3_camera.md port/ENGINE.md docs/PROGRESS.md docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md port/tests/test_engine_match_render.c
git commit -m "docs(fu72): record the activity wiring"
git push origin main
```
