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

Call `match_run_stats_seed(mr)` after the roster order is staged in `fifa96_match_run_begin` and in the act-9 reset; call `fifa96_match_stats_reset(&mr->stats)` in `fifa96_match_run_init`/`end` so a reused run starts clean. `match_run_replay_roster_id` is defined later in the file; add a static forward declaration above the seed helper if the compiler needs it.

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

Fresh-byte scope (Task 1 review correction):
- The ported first path (`0x77428..0x774D2`) has three live self arms —
  `0x77451` (`8ED40(rec, 0x64)`), `0x7749F` (`8ED40(rec, 0x19)`) and the
  tracked companion `0x77493` (`8ED40(tracked, 5)`) — plus the `0x774B2`
  `8ED40(rec, 5)` arm.
- The `0x774B2` arm is **dead** in the shipped image: the `0x77423` seat calls
  `0x71C94`, which calls `0x700F4`, and `0x700F4` zeroes `word[0x1577C0]` /
  `word[0x1577C2]` (`0x701e5..0x701ec`). The install tree's `p =
  word[0x1577C2] * dword[0x157754]` is therefore always 0 and the `JL
  0x774A6` arm is unreachable. Wire the self call from `stat_id` anyway (the
  same call covers the dead value 5); record the dead arm, do not fabricate a
  test for it.
- The second set of calls (`0x775FD`/`0x77613` in the `0x7759C..0x77631`
  fall-through tail) is inside the **unported** FU285-L8 tail; it stays a
  numbered leg. Do not wire it.

- [ ] **Step 1: Write the failing row-1B stats assertions**

Extend `test_action_1b_wide_route_binds_pan`
(`port/tests/test_engine_match_handlers.c:5520`): after the dispatch, assert
the wide route's `p >= 0` arm accumulated the tracked `+5` and self `+0x19`,
both at 2x, into the acting pitch slot:

```c
assert(f.mr.stats.value[0][0] == 60);   /* (5 + 0x19) * 2 */
assert(f.mr.render.replay.activity_count[0][0] == 60);
```

The seat set `cam->tracked = r->entity_id` (`0x71D27`), so both native calls
hit the same entity; 60 pins both calls (5-only would read 10, self-only 50).

Add a focused `test_action_1b_stats_phase10` cloned from the wide fixture with
`f.mr.state.phase = 0x10u` (the head still fails: it requires 0x0D) and
`sign_match == 0`; assert `match1b_stat_id == 0x64`, `stats.value[0][0] ==
200` (`0x64 * 2`) and `activity_count[0][0] == 200`.

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL — the stats value is still 0.

- [ ] **Step 3: Add the two calls**

In the `match1b_stat_id` block, after `stat_id`/`stat_tracked` are computed and
before they are stored in `mr->match1b_*`, emit them in native order (the
tracked `0x77493` precedes the self `0x7749F`):

```c
if (stat_tracked != 0u && cam->tracked != FIFA96_CAMERA_TRACKED_NONE)
  (void)fifa96_match_run_stats_add(mr, cam->tracked, 5);
if (stat_id != 0xFFu)
  (void)fifa96_match_run_stats_add(mr, r->entity_id, (int32_t)stat_id);
```

The single self call covers `0x77451` (+0x64), `0x7749F` (+0x19) and the dead
`0x774B2` (+5); the tracked call covers `0x77493` (+5). The `0x775FD`/`0x77613`
pair stays in the FU285-L8 tail leg.

- [ ] **Step 4: Run the test**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_handlers.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): accumulate row-1B activity"
```

---

### Task 3: Wire the row-09/08/0B sites

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c` (row-09 arm-1 event path, row-08 handler, row-0B arm 1)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: `fifa96_match_run_stats_add` from Task 1.
- Produces: the row-09 `+2`, row-08 `+2` and row-0B `-3` activity values.

Fresh-byte scope (Task 1 review correction):
- `0x80D6C`/`0x80D99` are the **row-09 arm-1 event path**
  (`match_row09_arm1`'s `0x80D18` block), not the target block.
  `0x80D6C` is `8ED40([0x157A83], 2)` (the controlled record, when its
  `byte[[ctl+0x28]] == 0x4A`); `0x80D99` is `8ED40(EBP, 2)` (the acting
  record, on the `[0x157A83] == NULL` / anim != 0x4A arms).
- `0x813CD`/`0x813F5` are **row-08's scan-hit arm** (`0x813A2`, inside
  `0x81068..0x814AF`), not row 09. The ported `fifa96_outfield_row08_step`
  exposes the branch as `out.events` + `out.user_row_4a`; the handler
  `fifa96_match_action_08` has the controlled id and the acting id.
  `0x813CD` is `8ED40(user, 2)`; `0x813F5` is `8ED40(rec, 2)`. The user arm is
  unreachable in the current model because `s.user_row_byte` is staged 0
  (OL-82) — wire it, but record it as a dead arm until the byte has a producer.
- `0x81BBE` is `8ED40(EBP, -3)` on the row-0B arm-1 impact tail.

- [ ] **Step 1: Write the failing stats assertions**

Row 09: add `test_action_09_arm1_stats` (do not extend
`test_action_09_arm1_installs`, whose first dispatch takes the target block).
Stage the `0x80D18` event path: `state.phase = 2`, `record.entity_id = 1`,
`active = 1`, `stage92 = 1`, `anim_id = 0x11`, `frame = 2`, `face7d = 0`,
`desc_c = 0`, `desc_16 = 0`, `render.camera.pos_y = 0`,
`team[0].records[1].lane_x = 0`, `lane_z = 0`, `cam_dz6f = 1`, `bound = 0`,
`entities.ball.pos_counter_f = 0`, `team[1].target = NONE`. (`cam_dz6f = 1`,
not 0: `fifa96_entity_angle(0,0)` returns `0x80`, which meets the `0x39`
threshold and takes the target block; the `(0,1)` pair makes the fold
`angle - face7d = 0`, below it.)

- Acting arm: `entities.controlled = FIFA96_MATCH_ENTITY_NONE`; assert
  `match09_event_id == 0x5D` and `stats.value[0][1] == 4` (`+2 * 2`).
- Controlled arm (fresh fixture): `entities.controlled = 2`,
  `team[0].records[2].anim_id = 0x4A`; assert `match09_event_id == 0x67`,
  `stats.value[0][2] == 4` and `stats.value[0][1] == 0`.

Row 08: extend `test_action_08_stage_flag_15877d`'s first dispatch (the
projection hit) with `stats.value[0][0] == 4` and
`render.replay.activity_count[0][0] == 4`.

Row 0B: extend `test_action_0b_duel_chain_installs_row_5e` after the third
dispatch (arm 1) with `stats.value[0][1] == -6` (`-3 * 2`).

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL — the stats values are still 0.

- [ ] **Step 3: Add the calls**

In `match_row09_arm1`, on both `0x80D8D` else arms (controlled present with
anim != 0x4A, and controlled absent):
```c
(void)fifa96_match_run_stats_add(mr, r->entity_id, 2);   /* 0x80D99 */
```
and in the `ctl != NULL && ctl->anim_id == 0x4Au` arm, before the event store:
```c
(void)fifa96_match_run_stats_add(mr, controlled, 2);     /* 0x80D6C */
```

In `fifa96_match_action_08`, after `fifa96_outfield_row08_step` returns:
```c
if (out.events != 0) {
  if (out.user_row_4a != 0u)
    (void)fifa96_match_run_stats_add(mr, controlled, 2); /* 0x813CD (dead, OL-82) */
  else
    (void)fifa96_match_run_stats_add(mr, id, 2);         /* 0x813F5 */
}
```

In `fifa96_match_action_0B`, before the `0x81BB7..0x81BBE` impact tail:
```c
(void)fifa96_match_run_stats_add(mr, r->entity_id, -3);  /* 0x81BBE */
```

- [ ] **Step 4: Run the test**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_handlers.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): accumulate row-09/08/0B activity"
```

---

### Task 3b: Wire the row-1A event-block sites

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_1A` event block, `0x7690C..0x76A3C`)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: `fifa96_match_run_stats_add` from Task 1.
- Produces: the row-1A `+0x64`, `+0xA` and `+5` activity values.

Fresh `get_xrefs_to 0x8ED40` finds three sites in the ported row-1A event block
(the FU-72 §2.2 list omitted them, and the fresh census is 35 sites, not 29):
- `0x7694F` — `8ED40(rec, 0x64)` on the `state.phase == 0x10` arm
  (`event_id = 5`);
- `0x76A04` — `8ED40(rec, 0xA)` on the `camera.event_param > 0xF0` bounce arm
  (`event_id = 1`);
- `0x76A35` — `8ED40(rec, 5)` on the final else arm (`event_id = 0`).
The `0x76960` (`[0x157C2C] == 0x5C`, event 4), `0x7697A` (`follow < 7`,
event 9) and `0x76A12` (`[0x157C2C] == 0x2D`, event 7) arms have no `8ED40`.

- [ ] **Step 1: Write the failing row-1A stats assertions**

Extend `test_action_1a_runs_body` (the `follow < 7` arm) with
`assert(f.mr.stats.value[0][0] == 0);` — that arm has no `0x8ED40`.

Add `test_action_1a_stats` with three fresh fixtures:
- `state.phase = 0x10`, the existing body staging → `match1a_event_id == 5`
  and `stats.value[0][0] == 200` (`0x64 * 2`).
- `state.phase = 0`, `camera.speed = 8` (follow >= 7),
  `camera.event_param = 0x100`, `bounce_15759f = 0x421`,
  `bounce_1575d0 = 0`, `bounce_1575cc = 0x2D1`, `global_157c2c = 0` →
  `match1a_event_id == 1` and `stats.value[0][0] == 20` (`0xA * 2`).
- `state.phase = 0`, `camera.speed = 8`, `camera.event_param = 0`,
  `global_157c2c = 0` → `match1a_event_id == 0` and
  `stats.value[0][0] == 10` (`5 * 2`).

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL — the stats values are still 0.

- [ ] **Step 3: Add the calls**

In the event-block branch chain, before each `event_id` store:
```c
(void)fifa96_match_run_stats_add(mr, r->entity_id, 0x64);  /* 0x7694F */
(void)fifa96_match_run_stats_add(mr, r->entity_id, 0xA);   /* 0x76A04 */
(void)fifa96_match_run_stats_add(mr, r->entity_id, 5);     /* 0x76A35 */
```
Only the phase-0x10, bounce and final-else arms get a call.

- [ ] **Step 4: Run the test**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_handlers.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): accumulate row-1A activity"
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

- FU-72: record the table/weight/wired sites and the remaining `8ED40` sites. A fresh `get_xrefs_to 0x8ED40` finds **35** direct calls, not the 29 in §2.2 — the list omitted `0x775FD`, `0x7694F`/`0x76A04`/`0x76A35` (row 1A) and `0x7ADD2`/`0x7AE04` (`FUN_0007A490`). After Task 3b, 12 addresses are wired (including the dead `0x774B2` and the OL-82-dead `0x813CD`); the remaining 23 are: 19 in unported camera/announce/row-1C bodies (`0x70F44`/`0x70FCE`, six `0x7131C`, `0x72DE3`/`0x72ED1`, four `0x72478`, four `0x77DFD`/`0x77E38`/`0x77E4E`/`0x77E61`, `0x8AD54`), 2 in the unported row-1B `0x7759C` tail (`0x775FD`/`0x77613`), and 2 in the unported `FUN_0007A490` tail (`0x7ADD2`/`0x7AE04`). Record the corrected census as a §2.2 erratum.
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
