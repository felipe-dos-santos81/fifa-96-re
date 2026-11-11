# Slot-Block Move Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port `FUN_000786A0` once as an engine helper and use it at every ported call site, so a slot hand-off moves the slot owner, the back-pointer, the slot dirs and the seven-word tail together.

**Architecture:** Add `fifa96_match_run_slot_move` beside the existing `fifa96_match_run_select_step` binder. The helper treats `mr->slot.entity` as the native `[rec+0x20]` pointer and `has_slot` as its record-visible projection; it heals the two on every move. Replace the four ad-hoc swap blocks (row 18, `0x7CD60`, `0x7D1D4`, set-piece cut) with the helper. The three native callers with no engine body and the `FUN_00093890` walk stay documented legs.

**Tech Stack:** C11, CMake/CTest, ASan/UBSan, Ghidra MCP evidence on `/FIFA96.EXE`.

**Spec:** `docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md` (Slice 1, §6)

## Global Constraints

- `make check` must stay green: configure + strict build (`-Wall -Wextra -Werror`) + the full CTest suite (currently 124/124).
- M1 must stay byte-exact against `port/tests/golden/engine/m1-frames.txt` (sha `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd`).
- M2 must stay byte-exact against `port/tests/golden/engine/m2-frames.txt` (295,089 bytes; current sha `41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`). Any movement is a stop, not a re-pin.
- Tests run from the repo root (`./build/test_engine_match_handlers`, etc.), except `test_engine_m2_noiso`, which CTest runs from the build dir.
- Work stays on `main` (repo `AGENTS.md` explicit consent); commit per task and `git push origin main`.
- Native address/byte claims cite the fresh Ghidra output in this plan; do not invent behavior for the three unported callers.
- Only `main` remains locally and remotely.

## Review Focus

The five failure modes most likely to bite, each with the task that pins it:

1. **From is not the slot holder** — `0x786A0` tests `[EAX+0x20] == 0` and returns; the helper must not move a record that merely has `has_slot == 1`. Pinned by Task 1's no-op case.
2. **Destination already holds the slot** — `0x786AC` rejects `[EDX+0x20] != 0`; the helper must not overwrite. Pinned by Task 1's no-op case.
3. **Self move** — `from == to` must not clear the tail or dirs. Pinned by Task 1.
4. **Two holders / stale donor dirs after a hand-off** — row 18 and the set-piece cut must leave one `has_slot`, one `slot.entity`, and the new holder's dirs. Pinned by Tasks 2 and 3.
5. **M2 tape movement** — the native state move is unconditional, but the consumer is live-gated; the M2 golden must not move. Pinned by Task 4's gate.

---

### Task 1: The slot-move helper

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (declare the helper beside `fifa96_match_run_select_step`)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (implement it beside `fifa96_match_run_select_step`)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: `struct fifa96_match_run` (`slot`, `entities`, `record`), `struct fifa96_match_entity` (`has_slot`, `dir_x`, `dir_z`), `fifa96_control_slot_merge_reset`.
- Produces: `int fifa96_match_run_slot_move(struct fifa96_match_run *mr, int32_t from_id, int32_t to_id, int clear_byte24);` — `1` moved, `0` native no-op, `-FIFA96_ERR_INVALID` on NULL.

- [ ] **Step 1: Write the failing helper test**

Add to `port/tests/test_engine_match_handlers.c`, before `int main(void)`:

```c
static void test_slot_move_helper(void) {
  struct fixture f;
  make_fixture(&f);
  struct fifa96_match_entity *from = &f.mr.entities.team[0].records[1];
  struct fifa96_match_entity *to = &f.mr.entities.team[0].records[2];
  from->has_slot = 1;
  to->has_slot = 0;
  from->dir_x = 3;
  from->dir_z = 4;
  f.mr.slot.entity = 1;
  f.mr.slot.anim_b = 7;
  f.mr.slot.anim_c = 8;
  f.mr.slot.pressed = 0x10;
  f.mr.slot.released = 0x20;
  f.mr.slot.held = 0x40;
  f.mr.slot.byte24 = 0x77;
  f.mr.record.entity_id = 1;
  f.mr.record.has_slot = 1;
  f.mr.record.dir_x = 3;
  f.mr.record.dir_z = 4;

  assert(fifa96_match_run_slot_move(&f.mr, 1, 2, 1) == 1);
  assert(from->has_slot == 0 && to->has_slot == 1);
  assert(f.mr.slot.entity == 2);
  assert(to->dir_x == 7 && to->dir_z == 8);
  assert(from->dir_x == 0 && from->dir_z == 0);
  assert(f.mr.slot.pressed == 0 && f.mr.slot.released == 0 &&
         f.mr.slot.held == 0);
  assert(f.mr.slot.byte24 == 0);
  assert(f.mr.record.has_slot == 0 && f.mr.record.dir_x == 0);
  assert(f.mr.record.dir_z == 0);

  /* The native no-ops: from is not the holder, the destination holds, self. */
  assert(fifa96_match_run_slot_move(&f.mr, 1, 0, 0) == 0);
  from->has_slot = 0;
  to->has_slot = 1;
  f.mr.slot.entity = 1;
  assert(fifa96_match_run_slot_move(&f.mr, 1, 2, 0) == 0);
  assert(fifa96_match_run_slot_move(&f.mr, 1, 1, 0) == 0);
  assert(fifa96_match_run_slot_move(NULL, 1, 2, 0) == -FIFA96_ERR_INVALID);
  drop_fixture(&f);
}
```

Call `test_slot_move_helper();` from `main` beside the other slot tests.

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL — implicit declaration of `fifa96_match_run_slot_move` (or a link error).

- [ ] **Step 3: Declare and implement the helper**

In `fifa96_match_run.h`, after the `fifa96_match_run_select_step` declaration, add:

```c
/* FU-275 L266-? / FU-315 L2: the full FUN_000786A0 hand-off. `mr->slot.entity`
 * is the native [rec+0x20] pointer; `has_slot` is its record-visible
 * projection. Moves the block's owner and back-pointer, clears the donor and
 * the seven tail words, keeps the dirs (the block moves wholesale), and clears
 * slot +0x24 only when `clear_byte24` (the 0x78B00 caller). Returns 1 moved,
 * 0 native no-op, -FIFA96_ERR_INVALID on NULL. */
int fifa96_match_run_slot_move(struct fifa96_match_run *mr, int32_t from_id,
                               int32_t to_id, int clear_byte24);
```

In `fifa96_match_run.c`, after `fifa96_match_run_select_step`, add:

```c
int fifa96_match_run_slot_move(struct fifa96_match_run *mr, int32_t from_id,
                               int32_t to_id, int clear_byte24) {
  struct fifa96_match_entity *from;
  struct fifa96_match_entity *to;
  uint32_t from_team, from_index, to_team, to_index;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (from_id < 0 || to_id < 0 || from_id == to_id) return 0;
  if (from_id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS) ||
      to_id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  from_team = (uint32_t)from_id / FIFA96_MATCH_ENTITY_RECORDS;
  from_index = (uint32_t)from_id % FIFA96_MATCH_ENTITY_RECORDS;
  to_team = (uint32_t)to_id / FIFA96_MATCH_ENTITY_RECORDS;
  to_index = (uint32_t)to_id % FIFA96_MATCH_ENTITY_RECORDS;
  from = &mr->entities.team[from_team].records[from_index];
  to = &mr->entities.team[to_team].records[to_index];
  /* 0x786A2/0x786AC: from must be the native pointer owner, to must be free. */
  if (mr->slot.entity != from_id) return 0;
  if (to->has_slot != 0 || mr->slot.entity == to_id) return 0;
  /* 0x786B0..0x786B5: the pointer moves; the slot back-pointer follows. */
  mr->slot.entity = to_id;
  from->has_slot = 0;
  to->has_slot = 1;
  /* 0x786BF..0x786E3: the seven tail words. The native clears the moved
   * block; the block is the engine's single slot. */
  (void)fifa96_control_slot_merge_reset(&mr->slot);
  if (clear_byte24) mr->slot.byte24 = 0;
  /* The dirs live in the block (+0x20/+0x21), so they survive the move; the
   * per-record copies are projections and must follow. */
  to->dir_x = (int8_t)mr->slot.anim_b;
  to->dir_z = (int8_t)mr->slot.anim_c;
  from->dir_x = 0;
  from->dir_z = 0;
  if (mr->record.entity_id == from_id) {
    mr->record.has_slot = 0;
    mr->record.dir_x = 0;
    mr->record.dir_z = 0;
  } else if (mr->record.entity_id == to_id) {
    mr->record.has_slot = 1;
    mr->record.dir_x = (int8_t)mr->slot.anim_b;
    mr->record.dir_z = (int8_t)mr->slot.anim_c;
  }
  return 1;
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS (`test_engine_match_handlers OK`).

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): add the native slot-block move helper"
```

---

### Task 2: Adopt the helper at row 18, `0x7CD60` and `0x7D1D4`

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_18`)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`match_input_handler` case `0x7CD60`, `fifa96_match_run_select_step`)
- Test: `port/tests/test_engine_match_handlers.c` (`test_action_18_runs_body`)
- Test: `port/tests/test_engine_match_input.c` (`test_control_switch_moves_slot`)

**Interfaces:**
- Consumes: `fifa96_match_run_slot_move` from Task 1.
- Produces: no new interface; the three call sites now share the native move.

- [ ] **Step 1: Write the failing row-18 assertions**

In `test_action_18_runs_body` (`port/tests/test_engine_match_handlers.c`), after the existing fixture setup and before the dispatch, stage the slot:

```c
f.mr.slot.entity = 0;
f.mr.slot.anim_b = 0x11;
f.mr.slot.anim_c = 0x22;
f.mr.slot.pressed = 0x40;
f.mr.slot.byte24 = 0x77;
```

After the existing `records[1].has_slot == 1` assertion, add:

```c
assert(f.mr.slot.entity == 1);                       /* 0x786A0 [[slot]] */
assert(f.mr.entities.team[0].records[1].dir_x == 0x11);
assert(f.mr.entities.team[0].records[1].dir_z == 0x22);
assert(f.mr.entities.team[0].records[0].dir_x == 0);
assert(f.mr.slot.pressed == 0);                      /* the seven-word tail */
assert(f.mr.slot.byte24 == 0x77);                    /* row 18 has no 0x78B00 */
assert(f.mr.record.has_slot == 0);
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL — `f.mr.slot.entity` is still 0 (the current row 18 swaps `has_slot` only).

- [ ] **Step 3: Replace the row-18 swap block**

In `fifa96_match_action_18`, replace the `if (found >= 0) { ... }` body's local `fifa96_action_duel_slot from/to` block and the three `has_slot` assignments with:

```c
if (found >= 0) {
  int32_t target_id = (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)found);
  (void)fifa96_match_run_slot_move(mr, id, target_id, 0);
  mr->record.has_slot = e->has_slot;
}
```

Keep the `target` pointer only if another line needs it; otherwise remove it. `id` is `mr->record.entity_id`.

- [ ] **Step 4: Run the row-18 test**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 5: Replace the `0x7CD60` swap**

In `match_input_handler` case `0x7CD60u`, replace the local `fifa96_action_duel_slot from/to` block and the `if (c->mr->slot.entity == c->id) ...` rebind with:

```c
(void)fifa96_match_run_slot_move(c->mr, c->id, team->target, 0);
```

The existing `target->has_slot != 0` gate stays before it. The `timer7cb`/`timer820` stores stay.

- [ ] **Step 6: Replace the `0x7D1D4` binder swap**

In `fifa96_match_run_select_step`, replace the `slot_owned` computation, the local `fifa96_action_duel_slot from/to` block, and the `if (slot_owned != 0 && out.slot_clear != 0)` block with:

```c
if (out.slot_clear != 0)
  (void)fifa96_match_run_slot_move(mr, id, out.new_controlled, 1);
```

The `out.clear_team_7cb` store stays before or after the move (the cells are disjoint; keep the native `0x7D33B` order before the slot clear). Remove `slot_owned` if it becomes unused.

- [ ] **Step 7: Add the select-step dirs assertions**

In `test_control_switch_moves_slot` (`port/tests/test_engine_match_input.c`), after `stage_select_common(&mr)` and before the press, set:

```c
mr.slot.anim_b = 0x31;
mr.slot.anim_c = 0x32;
```

After the existing `mate->has_slot == 1` assertion, add:

```c
assert(mate->dir_x == 0x31 && mate->dir_z == 0x32);
assert(owner->dir_x == 0 && owner->dir_z == 0);
```

- [ ] **Step 8: Run the engine tests**

Run: `cmake --build build --target test_engine_match_handlers test_engine_match_input -j"$(nproc)" && ./build/test_engine_match_handlers && ./build/test_engine_match_input`
Expected: both `OK`.

- [ ] **Step 9: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_handlers.c port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_handlers.c port/tests/test_engine_match_input.c
git commit -m "feat(engine): use the native slot move at row 18 and the input binders"
```

---

### Task 3: Adopt the helper at the set-piece receiver cut

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c` (`match_sp_receiver_cut`)
- Test: `port/tests/test_engine_match_handlers.c` (`test_action_10_receiver_cut`)

**Interfaces:**
- Consumes: `fifa96_match_run_slot_move` from Task 1.
- Produces: after the cut, the picked receiver holds the slot and the taker is free.

- [ ] **Step 1: Write the failing cut assertions**

In `test_action_10_receiver_cut`, after the second dispatch and the existing `camera_target` assertions, add:

```c
assert(f.mr.slot.entity == 2);                       /* the picked receiver */
assert(f.mr.entities.team[0].records[2].has_slot == 1);
assert(f.mr.entities.team[0].records[1].has_slot == 0);
assert(f.mr.entities.team[0].records[2].dir_x == 0x21);
assert(f.mr.entities.team[0].records[2].dir_z == 0x22);
assert(f.mr.slot.pressed == 0);
assert(f.mr.slot.byte24 == 0x55);
```

Before the second dispatch, stage the taker's slot:

```c
f.mr.slot.entity = 1;
f.mr.slot.anim_b = 0x21;
f.mr.slot.anim_c = 0x22;
f.mr.slot.pressed = 0x10;
f.mr.slot.byte24 = 0x55;
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL — `f.mr.slot.entity` is still 1 (the cut does not move the slot).

- [ ] **Step 3: Move the slot in the cut**

In `match_sp_receiver_cut`, replace the comment `/* 0x786A0's list move is not modeled; all of it stays FU275-L6. */` and add, before the `0x83937` camera-target write:

```c
/* 0x838EF: FUN_000786A0(taker, picked receiver). The native no-ops when the
 * taker is not the pointer owner; the camera target still follows the pick. */
(void)fifa96_match_run_slot_move(
    mr, r->entity_id,
    (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)pick), 0);
```

The `r->has_slot == 0` guard at the top stays (the native cut arm requires the taker's slot). The second native site `0x83AA0` has no distinct engine branch in the current cut/delivery code; record it with the unported callers in Task 4 unless implementation identifies the branch.

- [ ] **Step 4: Run the test to verify it passes**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_handlers.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): move the slot on the set-piece receiver cut"
```

---

### Task 4: Close the record and run the full gate

**Files:**
- Modify: `port/spec/FU275_taker_arm_residuals_2.md` (§4 Remaining)
- Modify: `port/spec/FU315_r2l6_claim_chain.md` (FU315-L2)
- Modify: `port/ENGINE.md` (the FU275-L6 line)
- Modify: `docs/PROGRESS.md` (phase 113)
- Modify: `docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md` (mark Slice 1 closed)

**Interfaces:**
- Consumes: Tasks 1–3 behavior.
- Produces: the recorded closure and the remaining-callers list.

- [ ] **Step 1: Update the records**

- `FU275_taker_arm_residuals_2.md` §4 Remaining: replace “All of `FUN_000786A0` … stays FU275-L6” with: the helper `fifa96_match_run_slot_move` is the full `0x786A0` body; row 18, `0x7CD60`, `0x7D1D4` and the `0x838EF` cut use it; the `0x74CD2`/`0x75451`/`0x76053` callers have no engine body, `0x938F0` (`FUN_00093890`) stays FU-70, and the second set-piece site `0x83AA0` has no distinct engine branch. FU315-L2 is closed.
- `FU315_r2l6_claim_chain.md` FU315-L2: mark closed by the helper; note the three unported callers.
- `ENGINE.md`: replace the “remaining leg is all of `FUN_000786A0`” sentence with the helper closure and the unported-caller list.
- `docs/PROGRESS.md`: add phase 113 with the helper, the four adopted sites, the tests and the gate numbers.
- Design doc: mark Slice 1 “closed” with the commit hashes.

- [ ] **Step 2: Run the full gate**

Run: `make check`
Expected: `100% tests passed, 0 tests failed out of 124`.

- [ ] **Step 3: Verify the tapes**

Run:

```bash
./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt && echo m1_exact
./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt && echo m2_exact
```

Expected: both exact.

- [ ] **Step 4: Run the windowed smoke**

Run: `timeout 20s env DISPLAY=:1 make game > /tmp/opencode/game-smoke-slot-move.log 2>&1; echo "smoke_exit=$?"`
Expected: `124` (timeout) with no error in the log.

- [ ] **Step 5: Commit and push**

```bash
git add port/spec/FU275_taker_arm_residuals_2.md port/spec/FU315_r2l6_claim_chain.md port/ENGINE.md docs/PROGRESS.md docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md
git commit -m "docs(engine): close the slot-block move"
git push origin main
```
