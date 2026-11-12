# Recorded-frame replay ring — implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Capture each recordable live frame into the native 150 × `0x118` ring and play it back through the native state machine, rendering the applied `0x155AA4` view instead of the frozen live pool.

**Architecture:** A new `fifa96_match_replay_ring.{c,h}` owns the byte-block record/view accessors, the pack/apply pair, the ring cursors, the composer, the playback steps, the camera pose and the render staging. `fifa96_match_run` integrates the capture into the live frame, the view staging into the frozen frame, the state machine and the camera pose. The record is a `uint8_t[0x118]` block, the view a `uint8_t[0x1D0]` block; the ring is embedded in the run.

**Tech Stack:** C11, CMake/CTest, ASan/UBSan, Ghidra (the native evidence is already recorded in the spec).

**Spec:** `docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md`

## Global Constraints

- C11, `-Wall -Wextra -Werror`; engine tests build with `-fsanitize=address,undefined -fno-sanitize-recover=all`.
- Baseline `make check` is 124/124. Task 1 adds `test_engine_replay_ring`, so from Task 1 the expected total is **125/125**; zero failures at every task boundary.
- `test_engine_m1` and `test_engine_m2` must `cmp` byte-exact against `port/tests/golden/engine/m1-frames.txt` / `m2-frames.txt`; no golden re-pin.
- Windowed smoke: `timeout 20s env DISPLAY=:1 make game` exits 124 with a clean log.
- No new dispatch body/row and no README title, port-table, census, ENGINE headline or completion-number movement.
- The ring is embedded (150 × `0x118`); no allocator/lifecycle.
- The record/view offsets in spec §5.1 are binding; tests pin raw bytes at those offsets.
- Docs are append-only; every native claim cites the spec's fresh evidence; anything unproved stays a numbered leg.
- Work on `main`; commit per task. The controller pushes after review.

## Review Focus

These are the input classes the spec implies but no single task's happy-path test would otherwise exercise. Each has a test in the owning task.

1. **Empty ring.** Arming before any record exists must show the current view, make every step a no-op, and exit cleanly. Test: `test_ring_empty` (Task 4), `test_ring_empty_reset` (Task 2).
2. **Held key through arm/exit.** The existing `input_primed`/`input_resync` must keep suppressing a spurious first edge and a post-exit press. Tests: `test_replay_arm_held_key`, `test_replay_switch_tackle_resync` (existing; rerun in Task 5).
3. **Full ring.** The 151st pack evicts the oldest record; `record_at(0)` is the second pack; the write cursor wraps to 0 and the read cursor to 1. Test: `test_ring_eviction` (Task 2).
4. **Phase switch mid-recording.** A reset phase clears the budget and packing resumes; no stale record is replayed. Test: `test_ring_capture_budget` (Task 2).
5. **Replay across a period/act boundary.** The ring is not reset per period; the exit resumes the live clock and entities. Tests: `test_replay_draws_recorded_past` plus the existing freeze/resume tests (Task 5).

## File Structure

- **Create** `port/src/fifa96_engine/fifa96_match_replay_ring.h` — constants, ring struct, public API.
- **Create** `port/src/fifa96_engine/fifa96_match_replay_ring.c` — view/record accessors, pack/apply, ring cursors, composer, playback, pose, stage.
- **Modify** `port/src/fifa96_engine/fifa96_match_run.h` — include the ring header, add the ring field and the `replay_view` byte block, rename the replay enum, declare the anim-bank helper.
- **Modify** `port/src/fifa96_engine/fifa96_match_run.c` — init/end ring reset, begin/step/exit integration, live capture, frozen view staging/pose, anim-bank helper, enum uses.
- **Create** `port/tests/test_engine_replay_ring.c` — Tasks 1–4 unit tests.
- **Modify** `port/tests/test_engine_match_frame.c` — Task 5 integration tests.
- **Modify** `port/tests/test_engine_match_render.c` — enum rename + view/pose tests.
- **Modify** `port/CMakeLists.txt` — the engine source and the new test target.
- **Modify** the docs in Task 6 — design closure, FU-108/109/112/FU-107/FU-111, ENGINE, PROGRESS.

---

### Task 1: Record/view blocks and the exact pack/apply pair

**Files:**
- Create: `port/src/fifa96_engine/fifa96_match_replay_ring.h`
- Create: `port/src/fifa96_engine/fifa96_match_replay_ring.c`
- Test: `port/tests/test_engine_replay_ring.c`
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (include the ring header; add the `replay_view` and `replay_ring` fields)
- Modify: `port/CMakeLists.txt` (add the source to `fifa96_engine`, add the test target)

**Interfaces:**
- Produces:
  - `#define FIFA96_MATCH_REPLAY_RING_RECORDS 150u`
  - `#define FIFA96_MATCH_REPLAY_RECORD_SIZE 0x118u`
  - `#define FIFA96_MATCH_REPLAY_VIEW_SIZE 0x1D0u`
  - `#define FIFA96_MATCH_REPLAY_VIEW_SLOTS 23u`
  - `#define FIFA96_MATCH_REPLAY_VIEW_HIDDEN_Y (-10000)  /* native dword 0xFFFFD8F0 */`
  - `struct fifa96_match_replay_ring { uint8_t record[150][0x118]; uint32_t read, write, count, stamp_acc, budget; uint8_t enabled; };`
  - `void fifa96_match_replay_pack(const uint8_t *view, uint8_t *record, uint16_t stamp);`
  - `void fifa96_match_replay_unpack(const uint8_t *record, uint8_t *view);`
- Consumes: nothing.

- [ ] **Step 1: Write the failing round-trip test and register the target**

In `port/tests/test_engine_replay_ring.c`, add `test_ring_pack_apply_roundtrip`. Fill a `uint8_t view[FIFA96_MATCH_REPLAY_VIEW_SIZE]` so the round trip can be exact:
- the position dwords at `0x0C + 0x0C*i`, `0x10 + 0x0C*i`, `0x14 + 0x0C*i` are sign-extended 16-bit values;
- the heading dwords at `0x120 + 4*i` are 16-bit values (`0..0xFFFF`);
- the focus dwords at `0x00/0x04/0x08` and the matrix/word fields are arbitrary distinct dwords/words;
- every other byte is `(uint8_t)(i * 7u + 3u)`;
- the word at `0x1CE` is `0xAAAA`.

Pack with `stamp = 0x1234` and assert the §5.1 map:
- `record[0x00..0x0B] == view[0x00..0x0B]`;
- for `i = 0..22`: `record[0x0C+2i]` = low word at `view[0x0C+0xCi]`, `record[0x3A+2i]` = low word at `view[0x14+0xCi]` (**z**), `record[0x68+2i]` = low word at `view[0x10+0xCi]` (**y**), `record[0x96+2i]` = low word at `view[0x120+4i]`; `record[0xC4+i] == view[0x17C+i]`; `record[0xDB+i] == view[0x193+i]`;
- `record[0xF2..0xF5] == view[0x1AA..0x1AD]`;
- `record[0xF6..0x10D] == view[0x1AE..0x1C5]`;
- `record[0x10E..0x113] == view[0x1C6..0x1CB]`;
- `record[0x114..0x115] == view[0x1CC..0x1CD]`;
- `record[0x116..0x117] == 0x34, 0x12`.

Clear an `applied[FIFA96_MATCH_REPLAY_VIEW_SIZE]`, call `fifa96_match_replay_unpack`, and assert `applied` equals `view` at every byte except `0x1CE/0x1CF`, which must be `0x34, 0x12`; assert the position dwords are sign-extended and the heading dwords' high words stay 0.

Register the test in `main()` with `puts("test_engine_replay_ring OK")` at the end. Add the CMake target now (same pattern as `test_engine_match_frame`):

```cmake
add_executable(test_engine_replay_ring tests/test_engine_replay_ring.c)
target_link_libraries(test_engine_replay_ring PRIVATE fifa96_engine)
target_include_directories(test_engine_replay_ring PRIVATE src)
target_compile_options(test_engine_replay_ring PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all)
target_link_options(test_engine_replay_ring PRIVATE -fsanitize=address,undefined)
add_test(NAME test_engine_replay_ring COMMAND test_engine_replay_ring
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..)
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_replay_ring -j"$(nproc)" && ./build/test_engine_replay_ring`
Expected: FAIL at compile/link because `fifa96_match_replay_pack`/`unpack` and the header do not exist.

- [ ] **Step 3: Implement the header and the pack/apply pair**

Create the header with the constants, the forward-declared `struct fifa96_match_run`, the ring struct and the two functions. In the `.c`, use byte-safe helpers (`memcpy` into `uint16_t`/`uint32_t`; the host is little-endian) and one 23-iteration loop:
- pack: copy the focus dwords, then each slot's low words to the three position arrays and the heading array; copy the appearance/frame byte lanes; copy the four camera bytes; copy the six matrix dwords; copy the six queue6 bytes; copy the `0x1CC` word; write the `stamp` at `0x116`.
- unpack: the inverse; write the stamp into the view's `0x1CE` word; leave the view's heading high words alone (the composer sets them to 0).

Include the ring header in `fifa96_match_run.h`; add
`uint8_t replay_view[FIFA96_MATCH_REPLAY_VIEW_SIZE];` to
`struct fifa96_match_run_replay` and `struct fifa96_match_replay_ring
replay_ring;` to `struct fifa96_match_run`. Add
`src/fifa96_engine/fifa96_match_replay_ring.c` to the `fifa96_engine` source
list.

- [ ] **Step 4: Run the test to verify it passes**

Run: `cmake --build build --target test_engine_replay_ring -j"$(nproc)" && ./build/test_engine_replay_ring`
Expected: PASS, `test_engine_replay_ring OK`.

- [ ] **Step 5: Run the full suite and commit**

Run: `make check` (expect 125/125, zero failures).

```bash
git add port/src/fifa96_engine/fifa96_match_replay_ring.h \
        port/src/fifa96_engine/fifa96_match_replay_ring.c \
        port/src/fifa96_engine/fifa96_match_run.h \
        port/tests/test_engine_replay_ring.c port/CMakeLists.txt
git commit -m "feat(engine): add the native replay record pack/apply pair"
```

---

### Task 2: Ring cursors, capture and the mode-window budget

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_replay_ring.h/.c`
- Test: `port/tests/test_engine_replay_ring.c`

**Interfaces:**
- Produces:
  - `void fifa96_match_replay_ring_reset(struct fifa96_match_run *mr);`
  - `void fifa96_match_replay_ring_record(struct fifa96_match_run *mr, int on);`
  - `void fifa96_match_replay_capture(struct fifa96_match_run *mr, uint32_t ticks);`
  - `uint32_t fifa96_match_replay_count(const struct fifa96_match_run *mr);`
- Consumes: `fifa96_match_replay_pack`, `mr->replay_ring`, `mr->render.replay.replay_view`, `mr->state.phase`.

- [ ] **Step 1: Write the failing capture/budget/eviction tests**

Add three tests to `test_engine_replay_ring.c` and call all three from `main()`:

`test_ring_capture_budget`:
- Init a run; `fifa96_match_replay_ring_reset(&mr); fifa96_match_replay_ring_record(&mr, 1);`
- Set `mr.state.phase = 0` (increment set). Put a distinct byte in `mr.render.replay.replay_view[0]` before each call.
- Call `fifa96_match_replay_capture(&mr, 1)` 90 times. Assert: after 2 calls `count == 1`; after 90 calls `count == 44`, `stamp_acc == 1`, and every record's word at `+0x116` is `2`.
- Set `mr.state.phase = 2` (reset set); call `capture` once. Assert `count == 45` (the reset clears the budget and the packer runs; the persisted accumulator makes this pack).
- Call `capture` 3 more times; assert `count == 46` (the next two ticks pack #46; the third leaves `stamp_acc == 1`).

`test_ring_eviction`:
- Reset/record; set `mr.state.phase = 2` (the reset set) for every call, so the budget is cleared to 0 and the packer always runs. Write the pack counter into `replay_view[0x17C]` (the slot-0 appearance lane, which packs to `record[0xC4]`); for each pack, call `capture(mr, 1)` twice (the packer needs two ticks), and repeat for 151 packs. Assert `count == 150`, `read == 1`, `write == 1`; `record[(1+0)%150]` (i.e. `record[1]`) has marker 2 at `+0xC4`; `record[(1+149)%150]` (i.e. `record[0]`) has marker 151.

`test_ring_empty_reset`:
- Reset; assert count 0, read/write 0, stamp 0, budget 0, enabled 0.
- `record(mr, 0)`; capture twice; assert count 0 (disabled).
- `record(mr, 1)`; `capture(mr, 1)` once; assert count 0, stamp_acc 1; `capture(mr, 1)` again; assert count 1, stamp 2.
- Reset; `record(mr, 1)`; `capture(mr, 0)` (the native flush); assert count 1, stamp_acc 0, and the record's word at `+0x116` is 1.
- `ring_reset`; assert all cursors/count/stamp/budget 0 and enabled 0.

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cmake --build build --target test_engine_replay_ring -j"$(nproc)" && ./build/test_engine_replay_ring`
Expected: FAIL at compile/link for the missing functions.

- [ ] **Step 3: Implement the ring cursors and capture**

`ring_reset`: zero `read`, `write`, `count`, `stamp_acc`, `budget`, `enabled`, and `mr->render.replay.cursor` / `cursor_limit` (the native `FUN_00064030` writes `[0x9A94] = 0`; the count mirror follows).
`ring_record`: set `enabled = on != 0`.
`capture`:
1. Budget: if `mr->state.phase` is in `{0,1,3,4,5,6,7,8,0xA,0xB,0xC,0x13,0x14}` increment `budget`; otherwise set `budget = 0`. If `budget >= 0x5A`, return **before** touching `stamp_acc`.
2. If `!enabled`, return.
3. If `ticks == 0`: `stamp_acc = 1` and **proceed to write** (the native flush). Otherwise `stamp_acc += ticks; if (stamp_acc < 2) return;`.
4. If `count > 0x95`: `read = (read + 1) % 150; count--;`
5. `pack(replay_view, record[write], (uint16_t)stamp_acc);`
6. `write = (write + 1) % 150; count++; stamp_acc = 0;`

`count` returns `mr->replay_ring.count` (0 on NULL).

- [ ] **Step 4: Run the tests to verify they pass**

Run: `cmake --build build --target test_engine_replay_ring -j"$(nproc)" && ./build/test_engine_replay_ring`
Expected: PASS.

- [ ] **Step 5: Run the full suite and commit**

Run: `make check` (expect 125/125).

```bash
git add port/src/fifa96_engine/fifa96_match_replay_ring.h \
        port/src/fifa96_engine/fifa96_match_replay_ring.c \
        port/tests/test_engine_replay_ring.c
git commit -m "feat(engine): capture the replay ring at the native cadence"
```

---

### Task 3: The composer (staged render slots → view block)

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_replay_ring.h/.c`
- Test: `port/tests/test_engine_replay_ring.c`

**Interfaces:**
- Produces: `void fifa96_match_replay_compose(struct fifa96_match_run *mr);`
- Consumes: `mr->render.entities[0..22].stage` (`pos`, `heading`, `anim_id`, `frame`, `hidden`), `mr->render.replay.replay_view`, `mr->render.ball_roll`, `mr->render.native_view_class`, `mr->render.native_view_valid[]`, `mr->render.native_view_cam[]`, `mr->render.action_cam`, `mr->render_focus_*`, `mr->camera_override_157a6f`, `mr->camera_override_triple`, `mr->render.camera.pos_*`, `mr->state.frame_delta`.

- [ ] **Step 1: Write the failing composer test**

Add `test_ring_compose` and call it from `main()`:
- Init a run; `running = 1`; `live_start = 1`; `state.frame_delta = 2`.
- Stage render slot 0: `pos = (11, 22, 33)`, `heading = 0x3FF << 16`, `anim_id = 7`, `frame = 4`, `hidden = 0`.
- Stage render slot 1: `hidden = 1`, position `(1,2,3)`.
- Stage render slot 22 (the ball): `pos = (44,55,66)`, `heading = 0x123 << 16`, `anim_id = 9`, `frame = 0`.
- Set `render.entity_count = 23`; `render.ball_roll = 0xA5`; `native_view_class = 0`; `native_view_valid[0] = 1`; `native_view_cam[0] = {pos (-10,20,30), yaw 0x111, pitch 0x222, ratio 0x333}`; `render_focus_valid = 1`; `render_focus_* = (100,200,300)`.
- Call `fifa96_match_replay_compose(&mr)`.
- Assert:
  - view `+0x00/+0x04/+0x08` = 100/200/300;
  - slot 0 x/y/z at `+0x0C/+0x10/+0x14` = 11/22/33; heading at `+0x120` = `((0x400 - 0x3FF) & 0x3FF) << 6`; appearance at `+0x17C` = 7; frame at `+0x193` = 4;
  - slot 1 y at `+0x10+0xC` = `0xFFFFD8F0`;
  - slot 22 x/y/z at `+0x0C+0xC*22`, `+0x10+0xC*22`, `+0x14+0xC*22` = 44/55/66; heading at `+0x120+4*22` = `((0x400 - 0x123) & 0x3FF) << 6`; appearance at `+0x17C+22` = 9; frame at `+0x193+22` = 0;
  - `+0x1AA` = 0xA5; `+0x1AB..+0x1AD` = 0; `+0x1C6..+0x1CB` = 0; `+0x1CC` = 1; `+0x1CE` = 2;
  - matrix **view** `+0x1AE..+0x1C5` (record `+0xF6..+0x10D`) = -10/20/30/0x111/0x222/0x333.
- Second block: `render_focus_valid = 0`; `camera_override_157a6f = 1`; `camera_override_triple = (9000, 20000, -9000)`; `native_view_class = 3`; `action_cam = {pos (1,2,3), yaw 4, pitch 5, ratio 6}`; call compose; assert the focus is clamped to `(-4000, 0, -4000)` (the native `|x|,|z| > 6000` arm and `y 0..16000` arm) and the matrix comes from `action_cam`.

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_replay_ring -j"$(nproc)" && ./build/test_engine_replay_ring`
Expected: FAIL at link/compile for `fifa96_match_replay_compose`.

- [ ] **Step 3: Implement the composer**

Fill the view exactly as spec §7.3 and §5.1:
- focus: `render_focus_*` when `render_focus_valid`, else `camera_override_triple` when `camera_override_157a6f`, else `render.camera.pos_*`; clamp with `fifa96_camera_focus_clamp` (±6000, 0..16000).
- slots 0..21: `render.entities[slot].stage`; x/y/z from `stage.pos`; packed heading `((0x400 - (stage.heading >> 16)) & 0x3FF) << 6`; appearance `stage.anim_id`; frame `stage.frame`; if `stage.hidden`, write `y = FIFA96_MATCH_REPLAY_VIEW_HIDDEN_Y`.
- slot 22: the ball's `render.entities[22].stage`; the same packed heading/appearance/frame derivation.
- `+0x1AA = render.ball_roll`; `+0x1AB..+0x1AD = 0`; `+0x1C6..+0x1CB = 0` (the six queue bytes, RR-L3); `+0x1CC = 1`; `+0x1CE = (uint16_t)state.frame_delta`.
- matrix from `native_view_cam[native_view_class]` when `native_view_class >= 0 && native_view_class < 3 && native_view_valid[native_view_class]`, else `action_cam`.

- [ ] **Step 4: Run the test to verify it passes**

Run: `cmake --build build --target test_engine_replay_ring -j"$(nproc)" && ./build/test_engine_replay_ring`
Expected: PASS.

- [ ] **Step 5: Run the full suite and commit**

Run: `make check` (expect 125/125).

```bash
git add port/src/fifa96_engine/fifa96_match_replay_ring.h \
        port/src/fifa96_engine/fifa96_match_replay_ring.c \
        port/tests/test_engine_replay_ring.c
git commit -m "feat(engine): compose the live frame into the replay view block"
```

---

### Task 4: Playback, the state machine and the native labels

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_replay_ring.h/.c`
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (enum, replay struct)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`replay_begin`/`replay_step`/`replay_exit`, `replay_progress`)
- Test: `port/tests/test_engine_replay_ring.c`
- Test: `port/tests/test_engine_match_render.c` (update the existing replay tests)
- Test: `port/tests/test_engine_match_frame.c` (update the existing freeze/phase tests that name the enum)

**Interfaces:**
- Produces:
  - `int fifa96_match_replay_apply(struct fifa96_match_run *mr, uint32_t index);`
  - `int fifa96_match_replay_step_forward(struct fifa96_match_run *mr);`
  - `int fifa96_match_replay_step_back(struct fifa96_match_run *mr);`
  - `uint16_t fifa96_match_replay_stamp(const struct fifa96_match_run *mr);`
  - `void fifa96_match_replay_pose(struct fifa96_match_run *mr);`
- The existing `fifa96_match_run_replay_step` loses its `delta` parameter:
  `int fifa96_match_run_replay_step(struct fifa96_match_run *mr, uint32_t buttons);`
  The ring tick is 1 inside the countdown; the Q8 frame delta stays in the
  scene staging, not the replay step.
- Enum (values are the wire contract): `IDLE = 0x82`, `PLAYING = 0x83`, `STEP_FWD = 0x84`, `STEP_BACK = 0x85`, `SLOW = 0x86`, `ARMED = 0x81`, `LIVE = 0`.
- Consumes: `unpack`, `replay_view`, `mr->render.replay.cursor/cursor_limit/advance/wait_timer/wait_reload`, `mr->replay_ring`.

- [ ] **Step 1: Write the failing playback tests**

Add to `test_engine_replay_ring.c` and call all of them from `main()`:

`test_ring_playback_cadence`:
- Fill the ring with 150 records, each with stamp `2` and a distinct marker (pack a view whose slot-0 appearance is the index, or write the record's `+0xC4` and `+0x116` directly); `read=0`, `write=0`, `count=150`.
- Set `replay.state = PLAYING`, `cursor=0`, `cursor_limit=150`, `advance=0`.
- Call `fifa96_match_run_replay_step(&mr, 0)` 300 times. Assert: after 2 calls `cursor == 1`; after 300 calls `cursor == 150`, `state == IDLE`; the applied view's marker is the last record.
- Repeat with `state = IDLE`, `cursor=0`: a step with `buttons=1` enters `PLAYING` and does **not** apply at `cursor 0 < count` (the native only rewinds/applies at the end); assert the view is unchanged and `cursor == 0`.

`test_ring_step_modes`:
- `state=IDLE`, `cursor=10`, `count=150`.
- `fifa96_match_run_replay_step(&mr, 8)`: assert `state == STEP_BACK (0x85)` and `cursor == 8` (two back steps); assert the applied view is record 8.
- `fifa96_match_run_replay_step(&mr, 0)`: assert `state == IDLE`.
- `fifa96_match_run_replay_step(&mr, 4)`: assert `state == STEP_FWD (0x84)` and `cursor == 10`.
- Set `wait_timer = 0`, `wait_reload = 3`; `fifa96_match_run_replay_step(&mr, 2)`: assert `state == SLOW`, `wait_timer == 3`, `cursor == 11` (the same call's SLOW case steps and reloads); a following `fifa96_match_run_replay_step(&mr, 2)` (the top resets to IDLE, the input re-enters SLOW) decrements `wait_timer` to 2 and does not step.
- `state=IDLE`, `cursor=count`: step with `buttons=1`; assert `state == PLAYING`, `cursor == 1` (the end rewind applied record 0).

`test_ring_empty`:
- A live, ACTIVE run with `replay_ring.count = 0`; `fifa96_match_run_replay_begin` returns 0; the first `replay_step` enters `IDLE` with `cursor == 0`; `buttons=1` enters `PLAYING`; `buttons=0` stays; `buttons=0x80` exits to `LIVE`/state 0; the view is untouched between steps.

Update the existing `test_engine_match_render.c` replay tests:
- `test_replay_begin_and_step`, `test_replay_step_machine`, `test_replay_begin_edges`, `test_replay_input_edges`, `test_replay_target`, `test_replay_row_and_draw` in `test_engine_match_render.c`, and the `FIFA96_MATCH_REPLAY_PLAY` uses in `test_engine_match_frame.c`: replace `FIFA96_MATCH_REPLAY_PLAY` with `IDLE`, `PAUSE` with `PLAYING`; drop the third `delta` argument from every `fifa96_match_run_replay_step` call; after button 8 assert `STEP_BACK` (0x85), after button 4 assert `STEP_FWD` (0x84), and the next step still asserts `IDLE`. These existing tests run with an empty ring (`count == 0`), so their cursor stays 0 — the cursor ±2 assertions belong to `test_ring_step_modes`, which fills `count = 150`. The existing `test_replay_switch_tackle_resync`/`test_replay_arm_held_key` assertions stay unchanged (the C1 behavior is preserved). `fifa96_match_run_replay_playing` still compares 0x82 (`IDLE`); `test_replay_state_helpers` stays green.

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cmake --build build --target test_engine_replay_ring test_engine_match_render -j"$(nproc)" && ./build/test_engine_replay_ring && ./build/test_engine_match_render`
Expected: FAIL — the enums/names and the step functions do not exist, and the old tests reference the old names.

- [ ] **Step 3: Implement the playback and the enum rename**

In `fifa96_match_replay_ring.c`:
- `apply`: return 0 when `index >= count` (or NULL); otherwise `unpack(record[(read+index)%150], replay_view); render.ball_roll = replay_view[0x1AA]; return 1;`.
- `step_forward`: if `cursor < count`: apply(`cursor`), `cursor++`; **inside that guard**, if `cursor >= count`, set state `IDLE`. At entry `cursor >= count` the step is a complete no-op and the state is untouched. `cursor_limit = count`; return 1 if applied.
- `step_back`: if `cursor > 0`: `cursor--`, apply(`cursor`); **inside that guard**, if `cursor < 1`, state `IDLE`. At `cursor == 0` the step is a no-op. `cursor_limit = count`; return 1 if applied.
- `stamp`: 0 when `cursor >= count`, else the `+0x116` word of `record[(read+cursor)%150]`.
- `pose`: if `camera_kind == FIFA96_CAMERA_REPLAY_POSE`, set `render_eye_*` from the **view** matrix `+0x1AE/+0x1B2/+0x1B6`, `render_eye_valid=1`, `render.yaw = +0x1BA`, `render.pitch = +0x1BE`, `render.view_ratio = +0x1C2` (the record's `+0xF6/+0xFA/+0xFE/+0x102/+0x106/+0x10A` lanes after unpack).

In `fifa96_match_run.h`: rename the enum values as above (the ring/view fields are from Task 1); change the `fifa96_match_run_replay_step` declaration to the two-argument `(mr, buttons)` form; keep `fifa96_match_run_replay_playing` comparing the **0x82** enumerator (`IDLE`): it is the native `FUN_0006401C` `[0x9A98] == 0x82` predicate, not the active-playing state.

In `fifa96_match_run.c`:
- `replay_begin`: clear `cursor`, `advance`; keep the C1 input reset; state ARMED.
- `replay_step`: change the signature to `(mr, buttons)` and update the frozen-frame caller in `fifa96_match_run_frame`; keep the `0x80 → ARMED` promotion; at the top, reset `STEP_FWD/STEP_BACK/SLOW` to `IDLE` (the native next-call reset); if ARMED, set `cursor=0`, apply record 0 when `count > 0` (which advances it to 1), leave `cursor=0` when the ring is empty, set `cursor_limit=count`, zero `advance`/`wait_timer`, select camera 0, target, state `IDLE`; `buttons==1` toggles `IDLE`/`PLAYING` with the end rewind; `buttons==8` sets `STEP_BACK`, `4` sets `STEP_FWD`, `2` sets `SLOW`; the `IDLE` case resets `wait_timer`; `PLAYING` adds the ring tick (1) to `advance` and runs the countdown loop (`stamp > 0 && stamp <= advance` → subtract, `step_forward`, re-read); `STEP_FWD/BACK` perform two forward/back calls; `SLOW` decrements the timer or steps/reloads; `0x80` exits. Add two `static` no-ops with comments: `replay_transition(mr)` for the native `FUN_000974D8` before the step/slow arms, and `replay_resync(mr)` for `FUN_0006428C` after each countdown step; both are RR-L10.
- `replay_progress` stays `cursor * 100 / cursor_limit` (the cells are now live).

- [ ] **Step 4: Run the tests to verify they pass**

Run: `cmake --build build --target test_engine_replay_ring test_engine_match_render -j"$(nproc)" && ./build/test_engine_replay_ring && ./build/test_engine_match_render`
Expected: PASS.

- [ ] **Step 5: Run the full suite and commit**

Run: `make check` (expect 125/125); M1/M2 `cmp` exact.

```bash
git add port/src/fifa96_engine/fifa96_match_replay_ring.h \
        port/src/fifa96_engine/fifa96_match_replay_ring.c \
        port/src/fifa96_engine/fifa96_match_run.h \
        port/src/fifa96_engine/fifa96_match_run.c \
        port/tests/test_engine_replay_ring.c \
        port/tests/test_engine_match_render.c \
        port/tests/test_engine_match_frame.c
git commit -m "feat(engine): play the recorded ring through the native states"
```

---

### Task 5: Frame capture, frozen view staging and the camera pose

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_replay_ring.h/.c`
- Modify: `port/src/fifa96_engine/fifa96_match_run.h/.c`
- Test: `port/tests/test_engine_match_frame.c`
- Test: `port/tests/test_engine_match_render.c`

**Interfaces:**
- Produces:
  - `void fifa96_match_replay_stage(struct fifa96_match_run *mr);`
  - `int fifa96_match_run_anim_bank(int32_t anim_id, uint8_t *bank_index);`
- Consumes: `fifa96_match_replay_compose/capture/pose`, `mr->render.replay.replay_view`, `mr->render.entities`, `mr->render.enabled`, `mr->live_start`.

- [ ] **Step 1: Write the failing integration tests**

Add to `test_engine_match_frame.c` and register them in its `main()` (and register `test_replay_view_render` in `test_engine_match_render.c`'s `main()`):

`test_replay_capture_in_frame`:
- A live, granted-frame fixture with `render.enabled = 1`, `live_start = 1`, `state.phase = 0`. Call `fifa96_match_run_begin` (the match begin, which enables the ring) or explicitly `fifa96_match_replay_ring_record(&mr, 1)`; do not rely on init alone, which leaves `enabled = 0`.
- Run 4 granted live frames via the file's `one_granted_frame` helper (or the pace loop). Assert `replay_ring.count == 2`, each record's stamp is 2, and the composed view's slot 0 position matches the live entity at the time of capture.

`test_replay_draws_recorded_past`:
- A live run with `render.enabled = 1`; call `fifa96_match_run_begin` (or `ring_record(&mr, 1)`) so the recorder is enabled. Run two granted frames with entity 0 at position A (the second frame packs A), then two granted frames with entity 0 at position B (the second packs B). The ring holds A then B.
- Arm the replay; run a granted frozen frame. Assert `render.entities[0].stage.pos` equals A (record 0), not B; assert the live entity's position is still B (the pool was not overwritten); exit; assert the next granted live frame resumes the clock (`tick_total` changes).

`test_replay_view_render` (in `test_engine_match_render.c`):
- Fill a record with distinct slot values and a matrix; apply it; with `live_start = 1` and `camera_kind = POSE`, call `fifa96_match_replay_pose` and `fifa96_match_replay_stage`; assert `render_eye_*`/`yaw`/`pitch`/`view_ratio` come from the record; assert the render slot's heading is folded once; assert `render.entities[i].bank_index` is re-derived from the applied appearance; assert the applied roll byte sets `render.ball_roll`; assert the live pool is unchanged.

Update the existing frozen-frame tests in the same step:
- `test_replay_frozen_live_view_applies_target`: the arm frame resets the camera to 0 (POSE), so select the non-POSE camera **after** the arm frame with `fifa96_match_run_replay_camera_set(&mr, 1)` (camera 1 = `FIFA96_CAMERA_REPLAY_VIEW`), then run another granted frozen frame and keep the existing eye/target assertions. Add a second block that selects camera 0 (POSE) and asserts the recorded matrix/eye wins and the driver does not overwrite it.
- `test_replay_frozen_scene_stage_keeps_record_anim`: seed a **record** and arm the replay (the `0x81` arm applies record 0), or set the view block and arrange `count == 0` so the arm leaves it; the stage now reads the view, not the live pool. Keep the assertion that the sim-visible record fields (`+0x28`/`+0x3D`) do not advance.

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cmake --build build --target test_engine_match_frame test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_frame && ./build/test_engine_match_render`
Expected: FAIL — `fifa96_match_replay_stage`/`fifa96_match_run_anim_bank` do not exist; capture is not wired into the frame.

- [ ] **Step 3: Implement the anim-bank helper, the stage and the frame hooks**

- Add `fifa96_match_run_anim_bank` in `fifa96_match_run.c` (wrapping the static `match_run_anim_rows` table via `fifa96_animation_row_lookup`; return 1 and set `*bank_index = row.sprite_bank` on success, 0 otherwise); declare it in `fifa96_match_run.h`.
- Add `fifa96_match_replay_stage`: loop **all 23 view slots** (0..22; the composer already filled slot 22 from the staged ball), read the view pos, unfold the heading `(0x400 - (packed >> 6)) & 0x3FF` into the high word, set `anim_id`/`frame`, `hidden = (y == FIFA96_MATCH_REPLAY_VIEW_HIDDEN_Y)`, re-derive `bank_index` with the helper; set `render.entity_count = 23`. Write render cells only.
- Live frame body: inside the granted loop, **inside** the
  `if (mr->render.enabled) { ... }` branch, after `match_run_scene_stage(mr)`,
  call `fifa96_match_replay_compose(mr); fifa96_match_replay_capture(mr, 1);`
  before the trace. The composer reads the staged render slots, so it must run
  after the staging; a headless run (`render.enabled == 0`) stages no slots and
  does not capture.
- Frozen frame: the `fifa96_match_run_replay_step` call becomes
  `fifa96_match_run_replay_step(mr, mr->render.replay.input_buttons)` (the
  `delta` parameter is gone); replace `match_run_scene_stage_positions(mr)`
  with `fifa96_match_replay_stage(mr)`; keep the pose feed; call
  `match_run_live_camera_follow` only when `mr->live_start &&
  replay.camera_kind != FIFA96_CAMERA_REPLAY_POSE`; call
  `fifa96_match_replay_pose(mr)` after the feed/driver and before the display
  update.
- `fifa96_match_run_init` calls `fifa96_match_replay_ring_reset` (gate off); the **match** `fifa96_match_run_begin` calls reset and then `fifa96_match_replay_ring_record(mr, 1)`; `fifa96_match_run_end` resets/clears the gate. `fifa96_match_run_replay_begin` does **not** reset the ring.

- [ ] **Step 4: Run the focused tests to verify they pass**

Run: `cmake --build build --target test_engine_match_frame test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_frame && ./build/test_engine_match_render`
Expected: PASS.

- [ ] **Step 5: Run the full gate and commit**

Run: `make check` (expect 125/125); `./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt`; same for m2; `timeout 20s env DISPLAY=:1 make game` (exit 124, clean log).

```bash
git add port/src/fifa96_engine/fifa96_match_replay_ring.h \
        port/src/fifa96_engine/fifa96_match_replay_ring.c \
        port/src/fifa96_engine/fifa96_match_run.h \
        port/src/fifa96_engine/fifa96_match_run.c \
        port/tests/test_engine_match_frame.c \
        port/tests/test_engine_match_render.c
git commit -m "feat(engine): render the recorded replay frame and capture live frames"
```

---

### Task 6: Closure records and the final gate

**Files:**
- Modify: `docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md` (status + closure section)
- Modify: `port/spec/FU108_replay_ring_buffer.md`
- Modify: `port/spec/FU109_replay_record_format.md`
- Modify: `port/spec/FU112_live_replay_block.md`
- Modify: `port/spec/FU107_mode_driver.md`
- Modify: `port/spec/FU111_replay_arming_queue_hud.md`
- Modify: `port/spec/FU-351_class3_camera.md` (the C3 record's ring leg is now landed)
- Modify: `port/spec/FU72_ring_stats.md` (the ring leg is now landed)
- Modify: `port/ENGINE.md`
- Modify: `docs/PROGRESS.md`

**Interfaces:**
- Consumes: Tasks 1–5.
- Produces: the append-only closure/errata record and the final gate evidence.

- [ ] **Step 1: Append the closure and errata records**

Append-only sections recording:
- the landed pack/apply, ring cursors, capture cadence and budget, composer, playback states, pose and stage;
- the tests and the gate;
- the errata from design §7.9: FU-108's `FUN_00063CBC` label; FU-109's byte lanes/axes/stamp asymmetry; FU-112's focus-vs-ball and `0x15887B`; FU-107/ENGINE's `0x82/0x83` and `0x84/0x85` labels; FU-111's budget cap (44 records per uninterrupted increment run);
- the remaining legs RR-L1..RR-L10;
- the note that no completion number moved (no new dispatch body/row) and no golden was re-pinned.

Update the design status to implemented (with the closure section). Do not rewrite prior sections.

- [ ] **Step 2: Run the final gate**

Run:
- `make check` → 125/125, strict-clean.
- `./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt`
- `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`
- `timeout 20s env DISPLAY=:1 make game` → exit 124 with a clean log.

- [ ] **Step 3: Commit**

```bash
git add docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md \
        port/spec/FU108_replay_ring_buffer.md port/spec/FU109_replay_record_format.md \
        port/spec/FU112_live_replay_block.md port/spec/FU107_mode_driver.md \
        port/spec/FU111_replay_arming_queue_hud.md \
        port/spec/FU-351_class3_camera.md port/spec/FU72_ring_stats.md \
        port/ENGINE.md docs/PROGRESS.md
git commit -m "docs: close the recorded-frame replay ring"
```

---

## Self-Review

- **Spec coverage:** §5.1 map → Task 1; §5.3 recorder/budget → Task 2; §5.2 composer → Task 3; §5.4 state machine/labels → Task 4; §5.5/§7.4/§7.5 pose/stage → Tasks 4–5; §7.6 tests → Tasks 1–5; §7.8 legs/§7.9 errata → Task 6; §7.7 acceptance → Task 5/6 gate.
- **Step scan:** every step is a test, a run, an implementation, a run or a commit; the offset maps and assertions carry the exact values.
- **Type consistency:** `fifa96_match_replay_pack`/`unpack`, `_ring_reset`/`_record`/`_capture`/`_count`, `_compose`, `_apply`/`_step_forward`/`_step_back`/`_stamp`/`_pose`/`_stage` and `fifa96_match_run_anim_bank` keep the same names and signatures across tasks.
- **Review Focus:** the five lines each name their test and owning task.
- **Proportion:** the plan gives signatures, values and assertions; the bodies remain the implementer's.
