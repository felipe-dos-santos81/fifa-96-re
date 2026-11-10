# FIFA 96 set-piece target chain implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close `FUN_00083428` / `FUN_00085498`'s native set-piece target path: the `sp` table, the three-point builder, the `0x158780` interpolation, the input-tail direction state, and the `0x157A6F` live camera focus.

**Architecture:** Pure path/interpolation helpers live in `fifa96_outfield`; the engine state lives on `fifa96_match_run` and `fifa96_match_team`; the `sp` table is loaded from `GAMEART0.PVI` and owned by the run; the live probe builds and steps the path; the camera follow consumes the override. Everything native is live-only so M1/M2 stay byte-exact.

**Tech Stack:** C11, CMake/CTest, ASan/UBSan engine tests, Ghidra MCP evidence.

**Spec:** `docs/superpowers/specs/2026-11-10-set-piece-target-chain-design.md`

## Global Constraints

- `make check` must stay **124/124**; strict build `-Wall -Wextra -Werror`.
- M1 sha `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd`; M2 sha `41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`; both `cmp` byte-exact.
- Native path and camera override run only when `live_start`; M2 keeps the derived timer/draw arm.
- ISO-gated tests skip cleanly when `data/game/FIFAPCCD96.iso` is absent; no test/tool writes the ISO.
- Every address/value claim cites a Ghidra read; unproven behavior is a numbered leg.
- Never synthesize an `sp` table; every table read is bounded by `sp_table_len`.
- Work on `main`; commit per task; push after the final gate.

## Review Focus

- A short or missing `sp` table must return no path, not read past the buffer.
- The last cell (`0x12A`) with direction 3 is out of bounds and must be rejected.
- Side 1 must negate the control offsets; side 0 must keep the table signs.
- Non-live/M2 must never build the native path or set the camera override.
- The camera override must clear on the gate-fail arm; non-set frames must keep the FU-71 place.

---

### Task 1: Pure path builder and interpolation

**Files:**
- Modify: `port/src/fifa96_loader/fifa96_outfield.h`
- Modify: `port/src/fifa96_loader/fifa96_outfield.c`
- Test: `port/tests/test_outfield.c`

**Interfaces:**
- Produces:
  - `#define FIFA96_SP_PATH_CELLS 0x12Bu`, `#define FIFA96_SP_PATH_STRIDE 12u`
  - `typedef struct fifa96_sp_target_state { uint8_t sub_index; int16_t accum; int16_t duration; int16_t vel_x; int16_t vel_z; int32_t target[3]; } fifa96_sp_target_state;`
  - `void fifa96_sp_build_path(int32_t focus_x, int32_t focus_z, const uint8_t *table, uint32_t table_len, int32_t cell_index, int32_t dir, uint8_t side, int32_t path[3][2], uint8_t *valid);`
  - `int16_t fifa96_sp_duration_from_metric(int32_t metric_shifted);`
  - `int fifa96_sp_target_step(fifa96_sp_target_state *state, const int32_t path[3][2], uint8_t valid, int32_t focus_x, int32_t focus_y, int32_t focus_z, int16_t delta);`

- [ ] **Step 1: Write the failing path-builder test**

Add to `port/tests/test_outfield.c`:

```c
static void test_sp_path_builder(void) {
  uint8_t table[3 * FIFA96_SP_PATH_STRIDE] = {0};
  int32_t path[3][2];
  uint8_t valid = 0;
  table[0] = 2; table[1] = (uint8_t)-3;
  table[2] = 4; table[3] = (uint8_t)-5;
  fifa96_sp_build_path(0x100, 0x200, table, sizeof table, 0, 0, 0, path, &valid);
  assert(valid == 1);
  assert(path[0][0] == 0x100 && path[0][1] == 0x200);
  assert(path[1][0] == 2 * 0x26 && path[1][1] == -3 * 0x21);
  assert(path[2][0] == 4 * 0x26 && path[2][1] == -5 * 0x21);
  fifa96_sp_build_path(0x100, 0x200, table, sizeof table, 0, 0, 1, path, &valid);
  assert(valid == 1);
  assert(path[1][0] == -2 * 0x26 && path[1][1] == 3 * 0x21);
  assert(path[2][0] == -4 * 0x26 && path[2][1] == 5 * 0x21);
  fifa96_sp_build_path(0, 0, table, sizeof table, 2, 3, 0, path, &valid);
  assert(valid == 0);
  fifa96_sp_build_path(0, 0, table, 3, 0, 0, 0, path, &valid);
  assert(valid == 0);
  fifa96_sp_build_path(0, 0, NULL, 0, 0, 0, 0, path, &valid);
  assert(valid == 0);
}
```

Call `test_sp_path_builder();` from `main` before `test_table_accessors();`.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build --target test_outfield -j"$(nproc)" && ./build/test_outfield`
Expected: FAIL with an undefined reference or assertion failure for `fifa96_sp_build_path`.

- [ ] **Step 3: Implement the path builder and duration helper**

In `fifa96_outfield.h`, add the two defines, the state struct and the three prototypes above.

In `fifa96_outfield.c`, implement:

- `fifa96_sp_build_path`: set `*valid = 0` on NULL/`dir` outside 0..3/`cell_index` outside 0..0x12A/`offset + 4 > table_len`; `path[0]` from the focus; `offset = cell_index * 12 + dir * 4`; copy `table[offset..offset+3]` as `int8_t` into `path[1]`/`path[2]` scaled by `0x26`/`0x21`; negate both points when `side != 0`; set `*valid = 1` on success.
- `fifa96_sp_duration_from_metric`: return `0x94` above `0x63F`, `3` below `1`, else `0x94 - table[(0x640 - v) >> 2]` clamped at 0. Use a static `uint8_t match_sp_duration_table[0x190]` copied from flat `0x10F4EE`.
- Add a static `int16_t match_sp_height_table[0x95]` copied from flat `0x10F3C4`.

- [ ] **Step 4: Run the path-builder test**

Run: `cmake --build build --target test_outfield -j"$(nproc)" && ./build/test_outfield`
Expected: PASS.

- [ ] **Step 5: Write the failing interpolation test**

Add to `port/tests/test_outfield.c`:

```c
static void test_sp_target_step(void) {
  int32_t path[3][2] = {{0, 0}, {0x26, 0}, {0, 0x21}};
  fifa96_sp_target_state st;
  memset(&st, 0, sizeof st);
  assert(fifa96_sp_duration_from_metric(2) == 8);
  assert(fifa96_sp_target_step(&st, path, 1, 0, 0x50, 0, 1) == 1);
  assert(st.sub_index == 1);
  assert(st.duration == 8);
  assert(st.vel_x == 2 && st.vel_z == 0);
  assert(st.target[0] == 2 && st.target[1] == 0 && st.target[2] == 0);
  assert(fifa96_sp_target_step(&st, path, 1, 0, 0x50, 0, 1) == 1);
  assert(st.sub_index == 1);
  assert(st.accum == 1);
  assert(fifa96_sp_target_step(&st, path, 0, 7, 9, 11, 1) == 0);
  assert(st.sub_index == 0 && st.target[0] == 7 && st.target[1] == 9 &&
         st.target[2] == 11);
}
```

Call it from `main`.

- [ ] **Step 6: Run test to verify it fails**

Run: `cmake --build build --target test_outfield -j"$(nproc)" && ./build/test_outfield`
Expected: FAIL for the missing `fifa96_sp_target_step`.

- [ ] **Step 7: Implement `fifa96_sp_target_step`**

Exact body:

- `valid == 0` or NULL state: reset `sub_index`/`accum`/`duration`/velocities and copy the focus into `target`; return 0.
- `sub_index == 0`: copy the focus into `target`, set `sub_index = 1`, load entry 1.
- else: `accum += delta`; if `accum > 2 * duration`, `sub_index++`; if `sub_index > 2`, set `sub_index = 1`; load entry `sub_index`.
- load entry `i`: `dx = (int16_t)(path[i][0] - target[0])`, `dz = (int16_t)(path[i][1] - target[2])`; `metric = fifa96_entity_distance(dx, dz)`; `duration = fifa96_sp_duration_from_metric(metric >> 4)`; if 0 set 8; `vel_x = dx / (2*duration)`, `vel_z = dz / (2*duration)`; `accum = 0`.
- always integrate: `target[0] += vel_x * delta`; `target[2] += vel_z * delta`; `i = abs(duration - accum)`; `target[1] = height[0x94 - i] - height[0x94 - duration]`, clamp negative to 0.
- return 1.

- [ ] **Step 8: Run the interpolation test**

Run: `cmake --build build --target test_outfield -j"$(nproc)" && ./build/test_outfield`
Expected: PASS.

- [ ] **Step 9: Commit**

```bash
git add port/src/fifa96_loader/fifa96_outfield.h port/src/fifa96_loader/fifa96_outfield.c port/tests/test_outfield.c
git commit -m "feat(engine): port set-piece path builder and interpolation"
```

---

### Task 2: `sp` bank entry loader and team table

**Files:**
- Modify: `port/src/fifa96_loader/fifa96_scene.h`
- Modify: `port/src/fifa96_loader/fifa96_scene.c`
- Modify: `port/src/fifa96_engine/fifa96_match_entities.h`
- Modify: `port/src/fifa96_engine/fifa96_match_run.c`
- Test: `port/tests/test_engine_team_data.c`

**Interfaces:**
- Consumes: `fifa96_bigf_parse`, `fifa96_bigf_record`, `fifa96_record_decode`.
- Produces:
  - `fifa96_err_t fifa96_scene_bank_entry_load(const uint8_t *file, size_t file_len, const char *name, uint8_t **out, size_t *out_len);`
  - `struct fifa96_match_team` fields `uint8_t *sp_table; uint32_t sp_table_len; uint8_t sp_table_ready;`

- [ ] **Step 1: Write the failing loader test**

Add to `port/tests/test_engine_team_data.c` a test that mounts the retail ISO, reads `/ART/GAMEART0.PVI`, calls `fifa96_scene_bank_entry_load(bank, bank_len, "352sp.fmt", &entry, &entry_len)`, and asserts `entry_len == 3588u`, `entry != NULL`, and `fifa96_sp_build_path(0, 0, entry, entry_len, 0, 0, 0, path, &valid) == 1`. Free `entry` and `bank`; skip the test when the ISO is absent, matching the file's existing `file_exists` guard.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build --target test_engine_team_data -j"$(nproc)" && ./build/test_engine_team_data`
Expected: FAIL for the missing `fifa96_scene_bank_entry_load`.

- [ ] **Step 3: Implement the loader**

In `fifa96_scene.c`, reuse the decode/BIGF path of `fifa96_scene_formation_load`, but copy the full entry into a `malloc(size)` buffer and return it through `*out`/`*out_len`; the caller frees it. Return `-FIFA96_ERR_INVALID` for NULL arguments, `FIFA96_ERR_NOT_FOUND` for a missing name, and `FIFA96_ERR_TRUNCATED` for a short entry.

- [ ] **Step 4: Run the loader test**

Run: `cmake --build build --target test_engine_team_data -j"$(nproc)" && ./build/test_engine_team_data`
Expected: PASS (or the ISO skip line when the ISO is absent).

- [ ] **Step 5: Add the team fields and the seed function**

In `fifa96_match_entities.h`, add the three `sp_table*` fields to `struct fifa96_match_team` with a comment naming `FUN_0006D9C4` / `FUN_0004C384(form*6+5)`.

In `fifa96_match_run.c`:

- Add the name table `{ "352sp.fmt", "442sp.fmt", "swsp.fmt", "424sp.fmt", "433sp.fmt" }`.
- Add `static void match_run_formation_sp_seed_team(struct fifa96_match_run *mr, uint32_t team, uint8_t id)`; it frees the old `sp_table`, reads `MATCH_RUN_FORMATION_BANK`, loads the slot-5 name, copies the entry, and sets `sp_table`/`sp_table_len`/`sp_table_ready`.
- Call it for both teams where `match_run_formation_seed` runs, using the same formation id the placement seed uses.
- Free both `sp_table` pointers in `fifa96_match_run_init` and `fifa96_match_run_end` (or the matching reset path).
- A missing bank/entry leaves `sp_table_ready = 0`; do not synthesize.

- [ ] **Step 6: Run the team-data test and the engine build**

Run: `cmake --build build --target test_engine_team_data test_engine_match_bridge -j"$(nproc)" && ./build/test_engine_team_data && ./build/test_engine_match_bridge`
Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add port/src/fifa96_loader/fifa96_scene.h port/src/fifa96_loader/fifa96_scene.c port/src/fifa96_engine/fifa96_match_entities.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_team_data.c
git commit -m "feat(engine): load native set-piece path tables"
```

---

### Task 3: Engine target state and input tail

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h`
- Modify: `port/src/fifa96_engine/fifa96_match_run.c`
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c`
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: `fifa96_sp_target_state` from Task 1.
- Produces on `struct fifa96_match_run`:
  - `fifa96_sp_target_state sp_target;`
  - `uint8_t sp_dir_15877b, sp_dir_15877f, sp_b5_1587b5;`
  - `int32_t sp_path[3][2]; uint8_t sp_path_valid;`

- [ ] **Step 1: Write the failing direction-tail test**

Add to `port/tests/test_engine_match_handlers.c`:

```c
static void test_sp_input_tail_direction(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.live_start = 1;
  f.mr.record.has_slot = 1;
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.code = 0x10;
  f.mr.record.stage92 = 2;
  f.mr.sp_b0_1587b0 = 1;
  f.mr.slot.entity = 1;
  f.mr.slot.raw = 1;
  f.mr.slot.prev_mapped = 0x40;
  f.mr.slot.pressed = 0;
  f.mr.slot.released = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x10) == FIFA96_OK);
  assert(f.mr.sp_dir_15877b == 1);   /* raw bit 1 -> direction 1 */
  assert(f.mr.sp_dir_15877f == 1);
  f.mr.slot.raw = 0;
  f.mr.slot.pressed = 0x80;
  assert(fifa96_match_dispatch_action(&f.mr, 0x10) == FIFA96_OK);
  assert(f.mr.sp_b5_1587b5 == 1);    /* pressed 0xB0 toggles b5 */
  drop_fixture(&f);
}
```

Call it from `main`.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL for the missing fields or the missing direction writes.

- [ ] **Step 3: Add the run state and reset**

In `fifa96_match_run.h`, include `fifa96_loader/fifa96_outfield.h` and add the fields above to `fifa96_match_run` with comments naming `0x158767/6B/6F`, `0x15877B._3`, `0x15877F`, `0x1587B5`.

In `fifa96_match_run.c`'s init/begin resets, zero `sp_target`, `sp_path`, `sp_path_valid`, `sp_dir_15877b = 3`, `sp_dir_15877f = 3`, `sp_b5_1587b5 = 0`.

- [ ] **Step 4: Extend the input tail**

In `match_sp_delivery_step` (`fifa96_match_handlers.c`), add the native writes:

- `pressed & 0x40` clears `sp_b3_1587b3` (already present).
- `state.phase != 8 && != 9 && (prev_mapped & 0x40)` and any raw bit `8/1/4/2` sets `sp_b3_1587b3 = 1` (already present).
- `(pressed & 0xB0) != 0` toggles `sp_b5_1587b5`.
- raw bit `8` -> `sp_dir_15877b = 3`, `sp_dir_15877f = 3`; bit `1` -> `1`/`1`; bit `4` -> `2`/`2`; bit `2` -> `0`/`0`; clear the low nibble of `slot.prev_mapped` (`slot.prev_mapped &= 0xFFF0u`) in the same branch.
- The `0x40` return path keeps `sp_dir_15877f` until the path build; do not reset it before the build.

- [ ] **Step 5: Run the handler test**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_match_handlers.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): stage set-piece target state and direction tail"
```

---

### Task 4: Build and step the native path

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c`
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: `fifa96_sp_build_path`, `fifa96_sp_target_step`, `fifa96_match_pitch_cell`, `team->sp_table`, `team->sp_table_len`, `team->sp_table_ready`.
- Produces: the live probe stages `sp_path`, `sp_path_valid`, `sp_target`.

- [ ] **Step 1: Write the failing live-path test**

Add a fixture with a synthetic 299×12 table. Put the non-zero control bytes at the cell the camera maps to, not blindly at cell 0:

```c
int32_t cell = fifa96_match_pitch_cell(f.mr.render.camera.pos_x,
                                       f.mr.render.camera.pos_z, 0);
uint32_t off = (uint32_t)cell * FIFA96_SP_PATH_STRIDE;
table[off + 0] = 2; table[off + 1] = 0;
table[off + 2] = 0; table[off + 3] = 1;
```

Use `sp_table_ready=1`, `sp_dir_15877b=0`, `live_start=1`, and the row-0x10 stage-2 slot fixture from `test_action_10_receiver_cut`. Dispatch twice and assert:

```c
assert(f.mr.sp_path_valid == 1);
assert(f.mr.sp_path[0][0] == f.mr.render.camera.pos_x);
assert(f.mr.sp_path[1][0] == 2 * 0x26);
assert(f.mr.sp_target.duration > 0);
assert(f.mr.sp_target.target[0] >= 0);
```

Add a non-live fixture with the same table and assert `sp_path_valid == 0` and `sp_target.target[0] == 0`.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL for the missing path build/step.

- [ ] **Step 3: Add the engine build helper**

In `fifa96_match_handlers.c`, add:

```c
static void match_sp_build_native_path(struct fifa96_match_run *mr,
                                       struct fifa96_match_run_record *r,
                                       uint8_t dir) {
  uint32_t team;
  struct fifa96_match_team *t;
  int32_t cell;
  if (r->entity_id < 0) { mr->sp_path_valid = 0; return; }
  team = (uint32_t)r->entity_id / FIFA96_MATCH_ENTITY_RECORDS;
  if (team >= FIFA96_MATCH_ENTITY_TEAMS) { mr->sp_path_valid = 0; return; }
  t = &mr->entities.team[team];
  if (!t->sp_table_ready || t->sp_table == NULL || t->sp_table_len == 0u) {
    mr->sp_path_valid = 0;
    return;
  }
  cell = fifa96_match_pitch_cell(mr->render.camera.pos_x,
                                 mr->render.camera.pos_z, t->side);
  fifa96_sp_build_path(mr->render.camera.pos_x, mr->render.camera.pos_z,
                       t->sp_table, t->sp_table_len, cell, dir, t->side,
                       mr->sp_path, &mr->sp_path_valid);
}
```

- [ ] **Step 4: Wire the native sites**

In `match_sp_probe_85498`'s live slot arm, first (before `match_sp_receiver_cut`, matching `FUN_00083428`'s head at `0x83441`):

- If `sp_b5_1587b5 == 0 && sp_dir_15877b <= 2`, call `match_sp_build_native_path(mr, &mr->record, sp_dir_15877b)`, then `fifa96_sp_target_step(&mr->sp_target, mr->sp_path, mr->sp_path_valid, mr->render.camera.pos_x, mr->render.camera.pos_y, mr->render.camera.pos_z, (int16_t)mr->record.delta)`. If the guard fails, clear `sp_path_valid` and keep the target at the focus.
- In `match_sp_receiver_cut`'s cut branch, call the build with `sp_dir_15877f` before setting the camera target; step once.
- In the `0x40` return path, call the build with `sp_dir_15877f` before resetting the sub-index.
- In the live no-slot arm, add `uint8_t throw_anim_7e7;` to `fifa96_match_team` (the native `[team+0x7E7]`), store the draw remainder there, build the path with that direction, and step.

For the non-live/M2 arm, leave `sp_path_valid = 0` and do not call the step.

- [ ] **Step 5: Run the handler test**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 6: Run the full gate and the tapes**

Run: `make check`
Expected: 124/124 passed.

Run: `./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt`
Expected: no output (byte-exact).

Run: `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`
Expected: no output (byte-exact).

- [ ] **Step 7: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_handlers.c port/src/fifa96_engine/fifa96_match_entities.h port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): build and step the native set-piece path"
```

---

### Task 5: Live camera focus override

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h`
- Modify: `port/src/fifa96_engine/fifa96_match_run.c`
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c`
- Test: `port/tests/test_engine_match_render.c`

**Interfaces:**
- Consumes: `sp_target.target[3]`, `sp_path_valid`, `sp_b5_1587b5`, `sp_dir_15877b`.
- Produces: `int32_t camera_override_triple[3];` and `camera_override_157a6f` used as a 0/1 flag.

- [ ] **Step 1: Write the failing camera-focus test**

Add to `port/tests/test_engine_match_render.c` a test that builds a live fixture, sets `mr.render.camera_override_157a6f = 1`, `camera_override_triple = {0x123, 0x456, 0x789}`, advances one live frame, and asserts `mr.render.render_focus_x == 0x123`, `render_focus_y == 0x456`, `render_focus_z == 0x789`. Clear the flag, advance one frame, and assert the focus is the FU-71 place (`mr.render.camera.pos_*`).

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: FAIL because the follow ignores the override.

- [ ] **Step 3: Add the override fields and consumer**

In `fifa96_match_run.h`, add `int32_t camera_override_triple[3];` next to `camera_override_157a6f`; keep `camera_override_157a6f` as the 0/1 active flag.

In `match_run_live_camera_follow`, select the source first, then apply the one clamp:

```c
int32_t src_x = r->camera_override_157a6f ? r->camera_override_triple[0]
                                          : cam->pos_x;
int32_t src_y = r->camera_override_157a6f ? r->camera_override_triple[1]
                                          : cam->pos_y;
int32_t src_z = r->camera_override_157a6f ? r->camera_override_triple[2]
                                          : cam->pos_z;
if (fifa96_camera_focus_clamp(src_x, src_y, src_z, &native_focus) != FIFA96_OK) {
  native_focus.x = src_x;
  native_focus.y = src_y;
  native_focus.z = src_z;
}
```

Keep the native `0x36C70` bounds on both sources and the existing fallback.

- [ ] **Step 4: Set and clear the override from the set-piece machine**

In `match_sp_probe_85498` / `match_sp_receiver_cut` / the `0x40` branch:

- When the path is valid and the guard passes, copy `sp_target.target[3]` into `camera_override_triple[3]` and set `camera_override_157a6f = 1`.
- On the gate-fail arm (`b5 != 0` or dir > 2), copy the FU-71 focus into `camera_override_triple` and clear the flag (the native `0x83629..0x8363D`).
- On init/begin/exit resets, clear the flag and zero the triple.

- [ ] **Step 5: Run the camera test**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_match_handlers.c port/tests/test_engine_match_render.c
git commit -m "feat(engine): consume the set-piece camera focus override"
```

---

### Task 6: Sinks, docs, full gate

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c`
- Modify: `port/spec/FU275_taker_arm_residuals_2.md`
- Modify: `port/spec/FU-351_class3_camera.md`
- Modify: `port/spec/FU154_live_camera_place_ball_visibility.md`
- Modify: `port/ENGINE.md`
- Modify: `docs/PROGRESS.md`
- Create: `docs/superpowers/plans/2026-11-10-fifa96-set-piece-target-chain.md` (this plan)
- Update: `.superpowers/sdd/2026-11-10-fifa96-camera-target-table/progress.md`

**Interfaces:**
- Consumes: the Task 1–5 behavior.
- Produces: the recorded closure and the remaining legs.

- [ ] **Step 1: Map the `FUN_000786A0` sink**

Check whether `picked.has_slot = 1` plus clearing the taker's list link represents the native move. If yes, implement it in the cut and add a test that the picked receiver is excluded from the next `fifa96_sp_pick_receiver`. If no, add a numbered leg (FU275-L6) and keep the visible exclusion behavior. Do not invent a list field.

- [ ] **Step 2: Add the `0x157A68` / `0x105FB0` resets**

Search the engine for the two cells. If either has no home, add a documented `uint8_t`/`int32_t` reset field to the run and write 0 at the cut, naming `FUN_0004C374` / `FUN_000361A4`. Add a test that the reset fields are 0 after the cut.

- [ ] **Step 3: Update the docs**

- `FU275` §3: the path builder, interpolation, input tail, no-slot build, camera override, and the remaining sink leg.
- `FU-351` §22: the `0x83937` row and the set-piece remaining sentence.
- `FU154`: close R2-L2 with the `0x157A6F` consumer.
- `ENGINE.md`: the `FUN_00083428`/`FUN_00085498` status and the new legs.
- `docs/PROGRESS.md`: phase 112 narrative with the measured gate.
- Plan/ledger: mark the tasks and the commit hashes.

- [ ] **Step 4: Run the full gate**

Run: `make check`
Expected: 124/124 passed.

Run: `./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt`
Expected: no output.

Run: `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`
Expected: no output.

Run: `timeout 20s env DISPLAY=:1 make game > /tmp/opencode/game-smoke-setpiece.log 2>&1; echo $?`
Expected: `124` (the timeout) with no error in the log.

- [ ] **Step 5: Commit and push**

```bash
git add port/src/fifa96_engine/fifa96_match_handlers.c port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_match_entities.h port/src/fifa96_loader/fifa96_scene.c port/src/fifa96_loader/fifa96_scene.h port/src/fifa96_loader/fifa96_outfield.c port/src/fifa96_loader/fifa96_outfield.h port/tests/test_outfield.c port/tests/test_engine_team_data.c port/tests/test_engine_match_handlers.c port/tests/test_engine_match_render.c port/spec/FU275_taker_arm_residuals_2.md port/spec/FU-351_class3_camera.md port/spec/FU154_live_camera_place_ball_visibility.md port/ENGINE.md docs/PROGRESS.md docs/superpowers/specs/2026-11-10-set-piece-target-chain-design.md docs/superpowers/plans/2026-11-10-fifa96-set-piece-target-chain.md
git commit -m "feat(engine): close the set-piece target chain"
git push origin main
```
