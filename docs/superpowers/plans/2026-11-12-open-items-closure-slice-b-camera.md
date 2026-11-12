# Slice B — camera/stats chain implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete the live camera spine and wire the Slice-B `0x8ED40` sites (camera chain, row-1C tail, `FUN_0008A938` arm) with native-order stats and no fabricated reachability.

**Architecture:** The loader camera module owns the pure bodies (`FUN_00070DE0`, `FUN_00072AC4` tail, the `FUN_000736AC` walk) and emits a typed stat-event array; the engine drains it into `fifa96_match_run_stats_add`. The engine owns the bodies that read run/entity state (`FUN_0007131C` post-arm continuation, `FUN_00072478`, row-1C tail, `FUN_0008A938`) and calls stats directly. The camera spine runs at the existing `match_run_frame_camera` / `fifa96_match_goal_arm` point, after the slot/camera update and before `match_run_entity_frame`.

**Tech Stack:** C11, CMake/CTest, ASan/UBSan (`-fno-sanitize-recover=all`), Ghidra MCP (`/FIFA96.EXE`), the null backend for determinism.

**Spec:** `docs/superpowers/specs/2026-11-12-open-items-closure-slice-b-design.md` (read §10 first: it corrects the site width, the `FUN_00072478` ownership, the counter guard, the reposition arm mapping, the save-tail start and the row-1C/case-6 routes).

## Global Constraints

- Work directly on `main`; commit per task (AGENTS.md). Commit types: `feat`, `fix`, `docs(fuNNN)`, `test`, `chore`.
- `make check` must stay green (125/125 plus new tests), strict `-Wall -Wextra -Werror`, engine tests under ASan/UBSan with `-fno-sanitize-recover=all`.
- `port/tests/golden/engine/m1-frames.txt` is **immutable**. M2 is compared after **every** behavioral landing (Tasks 3–11) and moves only per the design §9 per-incident ruling, with a written reason and frame diff.
- No completion-number movement unless a dispatch-row body appears; the camera bodies are not rows. If one is found to be a row, recompute the README/ENGINE numbers in the same commit.
- Docs are append-only: quote the superseded line; never rewrite a prior section.
- Every address/byte claim cites a fresh Ghidra read (`/FIFA96.EXE`); no stale `/tmp` image.
- Unsupported selectors `0x16`, `0x60`, `0x62`, `0x66`, `0x72`, `0x7A` stay UNSUPPORTED (FU-28).
- The four non-Slice-B `0x8ED40` sites (`0x775FD`/`0x77613`, `0x7ADD2`/`0x7AE04`) stay carried.
- A stat whose entity kind has no engine id is a numbered leg, never a fabricated id. Closure reports the actual wired count, not necessarily 19.

## Review Focus

- **Armer counter zero guard.** `[0x15781C] == 0` skips aging; a nonzero byte adds `frame_delta`, and only `> 0x1E` resets. Task 3 pins zero, nonzero, exactly 30, overflow and byte-wrap, plus an in-bounds frame that must not age.
- **Site width and entity resolution.** `site` is `uint32_t`; a kind the engine cannot resolve must be skipped and recorded, not folded into entity 0. Task 2's drain oracle and Tasks 8/10/11 pin it.
- **`FUN_00070DE0` arms.** `0x70F44` is `local_30 > 0x90`; `0x70FCE` is `local_30 <= 0x90` with `local_34 < -0xC0 || local_34 > 0xC1`. The existing step-back fixture reaches neither. Task 4 pins both plus the no-event control.
- **Native frame order.** `jitter -> 0x73723 FUN_00092864 -> 0x73728..0x73750 shadows -> 0x73756 FUN_00072AC4 -> 0x7375B prev capture -> 0x7375E pause`. Task 5/9 pin the commit/shadows and the save-tail split; a missing call is a numbered leg, not silent.
- **Jitter gate.** `speed < 0x19` runs the `(0x21 - speed)` modulo; `speed >= 0x19` bypasses it; then `draw & 7`; then two `draw & 1` velocity steps. Task 6 pins exact draws and ±1.
- **Six-arm placement, row-1C routes, case-6 entry.** The six `FUN_0007131C` sites are the post-arm continuation, not the wall/reflect helper; the row-1C tail has three separate routes; case 6 is the direct goal fallback, not `fifa96_match_run_situation(6)`. Tasks 7/10/11 pin each.

---

### Task 1: The Slice-B camera census

**Files:**
- Create: `docs/superpowers/specs/2026-11-12-slice-b-camera-census.md`
- Modify: `port/spec/FU72_ring_stats.md` (append a §"Slice B camera census (2026-11-12)" after the last section)
- Test: none (docs-only; verification is the grep/count check)

**Interfaces:**
- Produces: the census table consumed by Tasks 3–11. Columns: `site | body | native arm/gate | argument | delta | engine owner | port status | test`.
- Produces: the entity-kind table (`TRACKED`, `BALL_SUB7A6`, `REC_15B43C`, `REC_157A9B`, `LOCAL_EBP`, `REC_1575DA`) with the engine cell each resolves to, or `OPEN`.

- [ ] **Step 1: Fresh-read the 19 sites**

Run these Ghidra MCP reads on `/FIFA96.EXE`:
`get_xrefs_to 0x8ED40` (all direct calls), then `disassemble_function 0x72478` and `disassemble_function 0x72AC4` (the decompiler prunes reachable blocks in both; do not use `decompile_function` as the porting authority), `decompile_function 0x70DE0`, `decompile_function 0x7131C`, `disassemble_bytes 0x779DE` (the row-1C tail head), `disassemble_bytes 0x8AD2E` (the `MOV EBP,[0x157A9B]` and the case-6 arm). The 19 addresses are fixed: `0x70F44`, `0x70FCE`, `0x714BE`, `0x7157C`, `0x715DD`, `0x716C7`, `0x71732`, `0x717E3`, `0x72649`, `0x72660`, `0x7270F`, `0x72A52`, `0x72DE3`, `0x72ED1`, `0x77DFD`, `0x77E38`, `0x77E4E`, `0x77E61`, `0x8AD54`.

- [ ] **Step 2: Write the census table**

Create `docs/superpowers/specs/2026-11-12-slice-b-camera-census.md` with one row per address. For `FUN_00072478`, keep the FU-67 pairing: `0x72649 +0x32`, `0x72660 -0x4B` (`[ball[0]+0x7A6]`), `0x7270F +0x19` (`[0x15B43C]+5`), `0x72A52 +0xF` (previous phase `0x10`). For `FUN_00070DE0`, record `0x70F44` as the `local_30 > 0x90` arm and `0x70FCE` as the `local_30 <= 0x90` arm with `local_34 < -0xC0 || local_34 > 0xC1`. For the row-1C tail, record the three routes separately (phase-0x10 negative product; phase-non-0x10 nonnegative product; phase-non-0x10 negative product).

- [ ] **Step 3: Record the port status and entity mapping**

For each row, state `ported core` / `block missing` / `body missing` and the engine owner symbol. Correct the entity descriptions: `BALL_SUB7A6` is the pointer immediately after the team block's eleven `0xB2` records (FU-67 §…), not the tracked record's sub-object; `REC_15B43C` is a ring-record pointer whose `+5` field holds the entity; `REC_157A9B` is the backup-record pointer written by `FUN_00088940` at `0x8895B/0x88A1C/0x88AA4`; `LOCAL_EBP` is the handler's record. Write `OPEN` where the engine has no cell.

- [ ] **Step 4: Append the FU72 summary**

Append a short section quoting FU72's "Remaining sites (23)" line and recording that this plan covers the 19 Slice-B sites, while `0x775FD`/`0x77613` and `0x7ADD2`/`0x7AE04` stay carried.

- [ ] **Step 5: Verify and commit**

Run: `grep -c "| \`0x" docs/superpowers/specs/2026-11-12-slice-b-camera-census.md` (expect 19 rows), a duplicate check over the fixed list, and `git diff --check`.
Expected: 19 unique addresses, no whitespace errors.

```bash
git add docs/superpowers/specs/2026-11-12-slice-b-camera-census.md port/spec/FU72_ring_stats.md
git commit -m "docs: census the Slice B camera stats sites"
```

---

### Task 2: The typed stat-event contract

**Files:**
- Modify: `port/src/fifa96_loader/fifa96_camera.h` (add the enum/structs; extend `fifa96_camera_reposition`)
- Modify: `port/src/fifa96_loader/fifa96_camera.c` (`fifa96_camera_reposition` accepts the out-param; no emission yet)
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (declare `fifa96_match_run_stats_drain`)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (implement the drain)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Produces:

```c
typedef enum {
  FIFA96_CAMERA_STAT_TRACKED = 0,
  FIFA96_CAMERA_STAT_BALL_SUB7A6,
  FIFA96_CAMERA_STAT_REC_15B43C,
  FIFA96_CAMERA_STAT_REC_157A9B,
  FIFA96_CAMERA_STAT_LOCAL_EBP,
  FIFA96_CAMERA_STAT_REC_1575DA
} fifa96_camera_stat_entity;

typedef struct {
  uint32_t site;        /* the native CALL address; must hold 0x70F44 etc. */
  int16_t delta;
  uint8_t entity;
} fifa96_camera_stat_event;

#define FIFA96_CAMERA_STAT_MAX 8
typedef struct {
  fifa96_camera_stat_event event[FIFA96_CAMERA_STAT_MAX];
  uint8_t count;
  uint8_t overflow;
} fifa96_camera_stat_events;

int fifa96_match_run_stats_drain(struct fifa96_match_run *mr,
                                 const fifa96_camera_stat_events *events,
                                 int32_t local_ebp_id);
```

- `fifa96_camera_reposition` gains a final `fifa96_camera_stat_events *stats` parameter; `NULL` keeps the old behavior. `fifa96_match_run_stats_drain` returns the number of drained events; an unresolved kind is skipped (never mapped to entity 0).

- [ ] **Step 1: Write the failing drain test**

In `port/tests/test_engine_match_handlers.c`, add `test_stats_drain_events`: build a `struct fixture`, seed the stats table, construct two events `{0x70F44, 3, TRACKED}` and `{0x72DE3, 1, LOCAL_EBP}` with `count = 2`; set `f.mr.render.camera.tracked = 0` and pass `local_ebp_id = 1`; call `fifa96_match_run_stats_drain(&f.mr, &events, 1)`; assert the return is 2 and the two `stats.value` cells are `6` and `2`. Add a third event with the kind the census marks `OPEN` (currently `REC_157A9B`); assert the return is still 2 and the unresolved event did not touch the table. If the census resolves it, use a synthetic entity value above the enum range instead.

- [ ] **Step 2: Run it to verify it fails**

Run: `cmake --build build --target test_engine_match_handlers -j"$(nproc)" && ./build/test_engine_match_handlers`
Expected: FAIL — `fifa96_match_run_stats_drain` is undeclared.

- [ ] **Step 3: Add the types and the drain**

Add the enum/structs to `fifa96_camera.h`; add the `stats` parameter to `fifa96_camera_reposition` and update its three test calls to pass `NULL`. Implement the drain's `TRACKED`/`LOCAL_EBP` resolution and the census-resolved `REC_1575DA`/`BALL_SUB7A6`/`REC_15B43C`/`REC_157A9B` cells; skip `OPEN`. Call `fifa96_match_run_stats_add` in array order.

- [ ] **Step 4: Run the drain test and the suite**

Run: `cmake --build build --target test_camera test_engine_match_handlers -j"$(nproc)" && ./build/test_camera && ./build/test_engine_match_handlers`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_loader/fifa96_camera.h port/src/fifa96_loader/fifa96_camera.c port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_camera.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): add the camera stat-event contract"
```

---

### Task 3: The armer head and the reposition call

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (add `uint8_t goal_counter_15781c;` beside `goal_armed`)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`fifa96_match_goal_arm` head; init/clear sites)
- Test: `port/tests/test_engine_match_handlers.c`, `port/tests/test_engine_match_frame.c`

**Interfaces:**
- Consumes: `fifa96_camera_reposition(..., fifa96_camera_stat_events *stats)` and `fifa96_match_run_stats_drain` from Task 2.
- Produces: the native head `0x71325..0x71349`: read the byte; if zero, skip aging; else `counter = (uint8_t)(counter + frame_delta)`; if `counter > 0x1E`, set 0. Then call `fifa96_camera_reposition(&mr->render.camera, camera.prev_x, camera.prev_y, camera.prev_z, NULL, NULL, &events, &code, &stats)` and drain the events. The `0x71231` reseed is Task 4.

- [ ] **Step 1: Write the failing counter tests**

Add `test_goal_arm_counter_zero_guard`: start `goal_counter_15781c = 0`, `frame_delta = 5`, call `fifa96_match_goal_arm` seven times with the camera staged so the boundary path returns (the unit calls the armer directly; the counter head still runs); assert the counter stays `0` every time.
Add `test_goal_arm_counter_sequence`: start `5`, `frame_delta = 5`; assert `10, 15, 20, 25, 30, 0, 0` (30 is retained; 35 resets; the next zero stays zero).
Add `test_goal_arm_counter_exact_thirty`: start `0x1E`, `frame_delta = 0`; assert `0x1E` stays (the `> 0x1E` cap does not fire); then `frame_delta = 1` and assert `0`.
Add `test_goal_arm_counter_byte_wrap`: start `0xFF`, `frame_delta = 2`; assert `1`.

- [ ] **Step 2: Run them to verify they fail**

Run the handler target. Expected: FAIL — the field does not exist.

- [ ] **Step 3: Add the field and the native head**

Add the field; zero it in `fifa96_match_run_init`/`_begin`/`_end` beside `goal_armed`. In `fifa96_match_goal_arm`, before the `if (mr->goal_armed)` branch, implement the zero guard, byte add, strict `> 0x1E` reset, then the reposition call and drain. Do not move the existing branch logic.

- [ ] **Step 4: Run the counter tests**

Run the handler target. Expected: PASS.

- [ ] **Step 5: Write the failing in-bounds frame test**

In `test_engine_match_frame.c`, add `test_frame_goal_counter_in_bounds`: set `camera.pos_x = camera.pos_z = 0` (in bounds), `goal_counter_15781c = 5`, run `fifa96_match_run_frame`; assert the counter is still `5` (the native armer is called only through the out-of-bounds gate at `0x73B70..0x73B9B`).

- [ ] **Step 6: Run the frame test, then the M2 gate**

Run the frame target; then `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt; echo "m2_cmp=$?"`.
Expected: PASS; `m2_cmp=0`. If the armer fires on the M2 tape and the golden moves, **stop** and request the §9 per-incident ruling.

- [ ] **Step 7: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_handlers.c port/tests/test_engine_match_frame.c
git commit -m "feat(engine): run the armer head and reposition"
```

---

### Task 4: The `FUN_00070DE0` boundary arm and the two `+3` sites

**Files:**
- Modify: `port/src/fifa96_loader/fifa96_camera.h` (document the boundary arm; no signature change)
- Modify: `port/src/fifa96_loader/fifa96_camera.c` (`fifa96_camera_reposition`: port the bit-0x8 boundary arm, emit the two events, model the `0x71231` reseed)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (pass a stats array from `fifa96_match_goal_arm`; drain it — already wired in Task 3)
- Test: `port/tests/test_camera.c`

**Interfaces:**
- Produces: the two events `{0x70F44, 3, TRACKED}` (`local_30 > 0x90`) and `{0x70FCE, 3, TRACKED}` (`local_30 <= 0x90` and `local_34 < -0xC0 || local_34 > 0xC1`).
- Produces: the `0x71231` reseed of `[0x15781C]` from the low frame-delta byte when the counter is zero; if the reseed needs run state the loader does not own, it is a numbered leg and Task 3's zero-guard test remains the recorded behavior.

- [ ] **Step 1: Write the failing arm tests**

In `test_camera.c`, add `test_reposition_boundary_stat_high` and `test_reposition_boundary_stat_low`. The high fixture stages `local_30 > 0x90` (the `0x70F2C CMP [ESP+0x28],0x90` gate) and asserts one event `{0x70F44, 3, TRACKED}`. The low fixture stages `local_30 <= 0x90` and `local_34 <= -0xC1` (or `>= 0xC2`) and asserts `{0x70FCE, 3, TRACKED}`. Add `test_reposition_boundary_no_event` with `local_34` inside `[-0xC0, 0xC1]` and assert `count == 0`. Keep `test_reposition_steps_back` as the bit-0x8-clear no-event control.

- [ ] **Step 2: Run them to verify they fail**

Run the camera target. Expected: FAIL — the boundary arm is unported (the current derived core takes the bit-0x8-clear path).

- [ ] **Step 3: Port the boundary arm and emit the events**

Port the `0x70E3C..0x710FD` bit-0x8 boundary arm from `disassemble_function 0x70DE0`; the target z `±0xB11/±0xB0F`, the `0x8ED40`/`0x8F188` corner events, the target-y clamp. Emit the two events at the `0x70F44`/`0x70FCE` points only when `stats != NULL`. Model the `0x71231` reseed or record the leg in the census.

- [ ] **Step 4: Run the arm tests**

Run the camera target. Expected: PASS.

- [ ] **Step 5: Run the M1/M2 gate and commit**

Run: `make check`; `./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt`; `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`.
Expected: M1 exact; M2 exact (or the §9 stop). If the boundary arm is live on the M2 tape and the golden moves, stop.

```bash
git add port/src/fifa96_loader/fifa96_camera.h port/src/fifa96_loader/fifa96_camera.c port/tests/test_camera.c docs/superpowers/specs/2026-11-12-slice-b-camera-census.md
git commit -m "feat(engine): port the reposition boundary stats"
```

---

### Task 5: The `FUN_00092864` commit and the shadow stores

**Files:**
- Modify: `port/src/fifa96_loader/fifa96_camera.h` (declare `fifa96_camera_commit`)
- Modify: `port/src/fifa96_loader/fifa96_camera.c` (implement the camera-visible part)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`match_run_frame_camera`: jitter -> commit -> save head/tail -> walk)
- Test: `port/tests/test_camera.c`

**Interfaces:**
- Produces: `int fifa96_camera_commit(fifa96_camera *cam);` — the native `0x73723 CALL 0x92864` plus the `0x73728..0x73750` shadow stores `[0x157825] = word[0x1577BE]`, `[0x157827]`/`[0x157829]` from the speed/velocity pair. Add the shadow fields to `fifa96_camera` (`shadow_speed_157825`, `shadow_vel_157827`, `shadow_vel_157829`).
- If `FUN_00092864` has a non-camera effect the port cannot model (audio/queue), record it as a numbered leg; the shadow stores are still ported and tested.

- [ ] **Step 1: Write the failing commit test**

In `test_camera.c`, add `test_camera_commit_shadows`: stage `speed`, `vel_x`, `vel_z`; call `fifa96_camera_commit`; assert `shadow_speed_157825 == speed`, `shadow_vel_157827 == vel_x`, `shadow_vel_157829 == vel_z` (the native `AX=[0x1577BE]` copy).

- [ ] **Step 2: Run it to verify it fails**

Run the camera target. Expected: FAIL — undeclared.

- [ ] **Step 3: Implement the commit and shadows**

Port the camera-visible part; record the `FUN_00092864` non-camera effect as a leg if it cannot be modeled. Add the fields to `fifa96_camera` and zero them in `fifa96_camera_init`.

- [ ] **Step 4: Run the test**

Run the camera target. Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_loader/fifa96_camera.h port/src/fifa96_loader/fifa96_camera.c port/src/fifa96_engine/fifa96_match_run.c port/tests/test_camera.c
git commit -m "feat(engine): port the camera commit shadows"
```

---

### Task 6: The walk head jitter and the pause bridge

**Files:**
- Modify: `port/src/fifa96_loader/fifa96_camera.h` (declare `fifa96_camera_walk_jitter`)
- Modify: `port/src/fifa96_loader/fifa96_camera.c` (implement the jitter; keep the descent admission)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`match_run_frame_camera`: set `cam->paused = mr->flag_157820` before the walk; call the jitter first)
- Test: `port/tests/test_camera.c`, `port/tests/test_engine_match_frame.c`

**Interfaces:**
- Produces: `int fifa96_camera_walk_jitter(fifa96_camera *cam, struct fifa96_rng *rng);` — the native `0x736B2..0x73723` head:
  - `(int16)cam->speed <= 0xC` or `cam->pos_y <= 0xA0` -> no-op, no draws.
  - `speed < 0x19`: draw `FUN_00092AC8`, `EBX = 0x21 - speed`, `DIV EBX`; nonzero remainder -> no-op.
  - `speed >= 0x19`: skip the modulo.
  - draw `& 7`; nonzero -> no-op.
  - draw `& 1` -> `vel_x += 1` or `-= 1`; draw `& 1` -> `vel_z += 1` or `-= 1`.
- Consumes: `cam->paused` already exists; the engine sets it from `mr->flag_157820`.

- [ ] **Step 1: Write the failing jitter tests**

In `test_camera.c`, add `test_camera_walk_jitter_gates` (speed `0xC` and `pos_y = 0xA0` -> no change, no RNG consumption) and `test_camera_walk_jitter_draws` with a seeded RNG:
- `speed = 0x10` (`< 0x19`): the modulo draw is consumed; a nonzero remainder leaves the velocities unchanged.
- `speed = 0x20` (`>= 0x19`): the modulo draw is **not** consumed; the `& 7` and two `& 1` draws run.
- Assert the two velocities change by exactly ±1 on the accepted path, and `NULL` returns `-FIFA96_ERR_INVALID`.
Use the RNG seed that makes each branch deterministic; record the seed and the draws in the test comment.

- [ ] **Step 2: Run them to verify they fail**

Run the camera target. Expected: FAIL — undeclared.

- [ ] **Step 3: Implement the jitter**

Port `0x736B2..0x73723` exactly; use the same draw convention as `fifa96_camera_pan_step`. Keep the descent admission (`prev_y > 0 && pos_y <= 0`) in `fifa96_camera_update_walk` untouched.

- [ ] **Step 4: Run the jitter tests**

Run the camera target. Expected: PASS.

- [ ] **Step 5: Write the failing pause-bridge test**

In `test_engine_match_frame.c`, add `test_frame_camera_pause_bridge`: set `mr.flag_157820 = 1`, record `camera.pos_x/timer`, run `fifa96_match_run_frame`, assert the camera position/timer are unchanged while the frame is granted.

- [ ] **Step 6: Run it to verify it fails**

Run the frame target. Expected: FAIL — the walk still advances.

- [ ] **Step 7: Wire the bridge and the jitter**

In `match_run_frame_camera`: `fifa96_camera_walk_jitter`; set `mr->render.camera.paused = mr->flag_157820;`; `fifa96_camera_update_walk`. The order is `0x736B2` -> `0x73723` -> `0x7375E` (the commit/save calls are Tasks 5/9).

- [ ] **Step 8: Run the tests and the M2 gate**

Run the frame/camera targets; `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`.
Expected: PASS; `m2_cmp=0`. If not, stop for the §9 ruling.

- [ ] **Step 9: Commit**

```bash
git add port/src/fifa96_loader/fifa96_camera.h port/src/fifa96_loader/fifa96_camera.c port/src/fifa96_engine/fifa96_match_run.c port/tests/test_camera.c port/tests/test_engine_match_frame.c
git commit -m "feat(engine): bridge the camera pause and head jitter"
```

---

### Task 7: The `FUN_0007131C` post-arm continuation (six sites)

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`fifa96_match_goal_arm`: add a post-arm continuation after the arming success; do not change the already-armed/boundary branches)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Produces: `match_run_goal_post_arm(mr)` (static), called only on the successful arming path after the `0x713FC` continuation. It owns the six calls:
  - `0x714BE` `+5` — current phase 2, the `FUN_0008F078 > 0x1260`/tracked gates (`0x71495..0x714AD`).
  - `0x7157C` `-100` — current phase `0x10` (`0x714E0..0x714EB`).
  - `0x715DD` `-5` — the non-`0x10` branch with the `[0x15759F]`/`[0x1575A7]` gates.
  - `0x716C7` `-100` — previous phase `0x10` **or** current phase `0x10` (`0x7169E`/`0x716AB`).
  - `0x71732` `-5` — the other distance-gated branch.
  - `0x717E3` `+5` — the `[0x15758D]`/tracked-height/speed/RNG branch.
- Consumes: `fifa96_match_run_stats_add`; `[0x1577CA]` is `cam->tracked`.

- [ ] **Step 1: Write the six failing fixtures**

Add six tests beside `test_action_1b_stats_phase10` (`port/tests/test_engine_match_handlers.c:5641`): `test_goal_post_arm_plus5` (10), `test_goal_post_arm_neg100` (-200), `test_goal_post_arm_neg5` (-10), `test_goal_post_arm_prev_phase10_neg100` (-200, stage `state.prev_phase = 0x10`), `test_goal_post_arm_distance_neg5` (-10), `test_goal_post_arm_rng_plus5` (10). Each stages the exact gate from the census and asserts the `stats.value` cell and the `activity_count` mirror.

- [ ] **Step 2: Run them to verify they fail**

Run the handler target. Expected: FAIL — the post-arm body is absent.

- [ ] **Step 3: Implement the post-arm continuation**

Add the call after the arming success (`goal_armed = 1`, snapshot, zone) and before the function returns. Port the six gates in native order; keep the `FUN_0006E598`/`0x8F188` siblings as legs.

- [ ] **Step 4: Run the tests**

Run the handler target. Expected: PASS.

- [ ] **Step 5: Run the M2 gate and commit**

Run: `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`. If it moves, stop for the §9 ruling.

```bash
git add port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): wire the goal post-arm activity stats"
```

---

### Task 8: The `FUN_00072478` tracker decision (stat-only subset)

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (new `fifa96_match_tracker_decision`)
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (declare it)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Produces: `int fifa96_match_tracker_decision(struct fifa96_match_run *mr);` — the stat-bearing subset of `0x72478..0x72AC0`. The three `FUN_00092998` history searches and the `[0x15B43C]` producer at `0x7145E` are numbered legs (not silently faked).
- Produces: the four calls `0x72A52 +0xF`, `0x72649 +0x32`, `0x72660 -0x4B`, `0x7270F +0x19`.

- [ ] **Step 1: Write the failing previous-phase test**

Add `test_tracker_stats_prev_phase10`: set `state.prev_phase = 0x10`, `cam->tracked = 0`; call `fifa96_match_tracker_decision`; assert `stats.value[0][0] == 30`.

- [ ] **Step 2: Run it to verify it fails**

Run the handler target. Expected: FAIL — the body does not exist.

- [ ] **Step 3: Implement the head and the previous-phase arm**

Port the `0x72478..0x72AC0` decision path for this arm; keep the three `FUN_00092998` searches as legs. Do not call `FUN_0008ED40` on an unresolved entity.

- [ ] **Step 4: Run the test**

Run the handler target. Expected: PASS.

- [ ] **Step 5: Write the failing pair and staging tests**

Add `test_tracker_stats_fallthrough_pair`: stage the `0x725E8` gates; assert `stats.value` gains `100` and the `[ball[0]+0x7A6]` record gains `-150`. If the sub-object has no engine id, assert only the `+0x32` and record the leg. Add `test_tracker_stats_staging`: stage the `[0x15B43C]` record with `byte[0] == 0xB`; assert `50` (or the gate/leg if unresolved).

- [ ] **Step 6: Implement the pair and the staging arm**

Port `0x725E8..0x72660` and `0x726E9..0x727D7`. Use `fifa96_match_run_stats_add`; skip an unresolved kind.

- [ ] **Step 7: Run the tests and the M2 gate**

Run the handler target; `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`. If it moves, stop for the §9 ruling.

- [ ] **Step 8: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_match_run.h port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): port the tracker decision stats"
```

---

### Task 9: The `FUN_00072AC4` save tail and the frame order

**Files:**
- Modify: `port/src/fifa96_loader/fifa96_camera.h` (declare `fifa96_camera_save_tail`)
- Modify: `port/src/fifa96_loader/fifa96_camera.c` (implement the tail; emit the two events)
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (add `uint8_t tracker_15759d;`)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`match_run_frame_camera`: jitter -> commit -> save-head gate -> tracker -> save tail -> walk)
- Test: `port/tests/test_camera.c`, `port/tests/test_engine_match_frame.c`

**Interfaces:**
- Produces: `int fifa96_camera_save_tail(fifa96_camera *cam, fifa96_camera_stat_events *stats);` — the `0x72AC4` body **after** the `0x72AE5 CALL 0x72478`, from `0x72AEA` through `0x736A9`. The `FUN_000721C8` (`0x72B19`) and `FUN_00072270` (`0x7369E`) calls are legs. Emits `{0x72DE3, +1, REC_1575DA}` and `{0x72ED1, +2, REC_1575DA}`.
- The save-head gate stays in the engine: `if (goal_zone == 0 || goal_armed == 0) tracker_15759d = 0; else if (tracker_15759d == 0) { fifa96_match_tracker_decision(mr); tracker_15759d = 1; }`.
- Consumes: `fifa96_camera_walk_jitter` (Task 6), `fifa96_camera_commit` (Task 5), `fifa96_match_tracker_decision` (Task 8).

- [ ] **Step 1: Write the failing save-tail test**

In `test_camera.c`, add `test_camera_save_tail`: stage the `0x72DE3` and `0x72ED1` gates; call with a stats array; assert the event sites/deltas/entity and the tail's camera state.

- [ ] **Step 2: Run it to verify it fails**

Run the camera target. Expected: FAIL — undeclared.

- [ ] **Step 3: Implement the tail from `0x72AEA`**

Port `0x72AEA..0x736A9`; do not call the engine. Keep `FUN_000721C8`/`FUN_00072270` as legs. Emit the two events.

- [ ] **Step 4: Run the save-tail test**

Run the camera target. Expected: PASS.

- [ ] **Step 5: Write the failing frame-order test**

In `test_engine_match_frame.c`, add `test_frame_camera_order`: stage `goal_zone != 0`, `goal_armed != 0`, `tracker_15759d = 0`, a tracker decision that records a stat; run the frame; assert the tracker stat landed before the walk's prev capture and that `tracker_15759d` stays 1 on the next frame; assert `goal_zone == 0` clears it.

- [ ] **Step 6: Run it to verify it fails**

Run the frame target. Expected: FAIL — the save/tracker calls are absent.

- [ ] **Step 7: Wire the frame order**

`fifa96_camera_walk_jitter`; `fifa96_camera_commit`; the save-head gate; `fifa96_camera_save_tail` + drain; set `cam->paused`; `fifa96_camera_update_walk`.

- [ ] **Step 8: Run the tests and the M2 gate**

Run the frame/camera/handler targets; `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`. If it moves, stop.

- [ ] **Step 9: Commit**

```bash
git add port/src/fifa96_loader/fifa96_camera.h port/src/fifa96_loader/fifa96_camera.c port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_camera.c port/tests/test_engine_match_frame.c
git commit -m "feat(engine): port the camera save tail"
```

---

### Task 10: The row-1C tail (three stat routes)

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_1C`, the `0x779DE..0x77E8B` tail)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Produces: the three routes, with `word[rec+0x81] == 0` required to enter the tail:
  - Phase `0x10`, `(int16)vel_x * cam_z < 0`: `0x77DFD` `+0x64` -> 200; no `0x19`/`5`.
  - Phase != `0x10`, product >= 0: `0x77E38` `+0x19` -> 50; if `[0x1577CA] != 0`, `0x77E4E` `+5` -> 10.
  - Phase != `0x10`, product < 0: `0x77E61` `+5` -> 10.
  - Phase `0x10`, product >= 0: no stat.

- [ ] **Step 1: Write the failing route tests**

Add `test_action_1c_tail_phase10_negative` (200, no later stats), `test_action_1c_tail_nonphase10_positive` (50 and 10 when tracked), `test_action_1c_tail_nonphase10_negative` (10 only), and `test_action_1c_tail_phase10_nonnegative` (0). Each fixture sets `record.timer81 = 0` and the route's phase/product; assert the exact `stats.value` cells.

- [ ] **Step 2: Run them to verify they fail**

Run the handler target. Expected: FAIL — the tail returns at `0x779DE`.

- [ ] **Step 3: Port the tail**

Port `0x779DE..0x77E8B`; keep the `0x8F188`/anim siblings as legs. Wire the three routes in native order.

- [ ] **Step 4: Run the tests and the M2 gate**

Run the handler target; `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`. If it moves, stop.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_handlers.c port/tests/test_engine_match_handlers.c
git commit -m "feat(engine): port the row-1C tail stats"
```

---

### Task 11: The `FUN_0008A938` direct-goal arm

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`match_run_goal_fallback`, case 6)
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (a public test seam if the fallback is static)
- Test: `port/tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: `fifa96_match_run_stats_add`; the direct-goal fallback, not `fifa96_match_run_situation(6)`.
- Produces: the non-NULL branch `stats_add([0x157A9B], -0x96)` (2x -> `-300`) and the NULL branch's existing `stats_bump`.

- [ ] **Step 1: Prove the `[0x157A9B]` producer**

Fresh-read `get_xrefs_to 0x157A9B` (writers `FUN_00088940` at `0x8895B/0x88A1C/0x88AA4`) and record whether the engine has a matching backup-record cell. Write the result in the census doc. If the producer is unproved, stop here and record the open leg (no test, no fabricated -300).

- [ ] **Step 2: Write the failing direct-fallback test**

If live: add a public seam for `match_run_goal_fallback` and stage `[0x157A9B]` non-NULL with a valid record; assert `stats.value == -300` and that `stats_bump` was not called.
If proved unreachable: add `test_case6_157a9b_gate` asserting the branch gate is never entered on the engine's live path, and record the dead-arm proof.

- [ ] **Step 3: Run it to verify it fails**

Run the handler target. Expected: FAIL — the arm is unported.

- [ ] **Step 4: Port the arm**

Port `0x8AD54` in the direct-goal fallback. Preserve the non-NULL `-0x96` vs NULL `stats_bump` split.

- [ ] **Step 5: Run the test and the M2 gate**

Run the handler target; `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`. If it moves, stop.

- [ ] **Step 6: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_match_run.h port/tests/test_engine_match_handlers.c docs/superpowers/specs/2026-11-12-slice-b-camera-census.md
git commit -m "feat(engine): port the direct-goal backup-record arm"
```

---

### Task 12: Closure and gate

**Files:**
- Modify: `port/ENGINE.md`, `docs/PROGRESS.md`
- Modify: `port/spec/FU72_ring_stats.md`, `port/spec/FU-351_class3_camera.md`, `port/spec/FU145_goal_arming.md`, `port/spec/FU164_pan_arm_installers.md`
- Test: the whole suite

**Interfaces:**
- Consumes: Tasks 1–11.

- [ ] **Step 1: Append the closure records**

Record the actual wired count (not necessarily 19), the carried/open kinds, the armer head and reseed, the reposition boundary arm, the commit/shadows, the jitter/pause, the six post-arm sites, the tracker stat-only subset and its legs, the save tail and frame order, the row-1C routes, and the case-6 outcome. Quote the superseded leg lines (FU145 L1, FU164 §2, FU186-L2, FU72 "Remaining sites (23)").

- [ ] **Step 2: Recompute the three numbers**

Run `ctest --test-dir build -R test_engine_match_port_census --output-on-failure`. Confirm 78/78 and 61/80/64/80 (or recompute if a row body appeared). Update the README/ENGINE only if they moved.

- [ ] **Step 3: Run the full gate**

`make check`; `./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt`; `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`; `timeout 20s env DISPLAY=:1 make game > /tmp/opencode/slice-b-smoke.log 2>&1; echo "smoke_exit=$?"`.
Expected: all tests pass; `m1_cmp=0`; `m2_cmp=0` (or the approved diff); `smoke_exit=124` with a clean log.

- [ ] **Step 4: Commit**

```bash
git add port/ENGINE.md docs/PROGRESS.md port/spec/FU72_ring_stats.md port/spec/FU-351_class3_camera.md port/spec/FU145_goal_arming.md port/spec/FU164_pan_arm_installers.md
git commit -m "docs: close the Slice B camera/stats chain"
```

---

## Self-Review

**Spec coverage:** design §2 goals 1–2 (the 19 sites and the spine) -> Tasks 1–11; goal 5's carried-four note -> Task 1 step 4 and Task 12 step 1. The replay (B7–B8) and remap (B9–B10) goals are separate plans; the design §10 corrections are folded into Tasks 2–11.

**Step scan:** Every step names a file, a test, a command, or a native address. The body control flow is referenced to the census/disassembly; the signatures and stat values are pinned. The plan no longer claims full-body ports where only a stat-bearing subset is in scope (Tasks 8/9 say so explicitly).

**Type consistency:** `uint32_t site` (Task 2), `fifa96_camera_stat_events` (Tasks 2/4/9), `fifa96_camera_commit` (Task 5/9), `fifa96_camera_walk_jitter` (Task 6/9), `fifa96_match_tracker_decision` (Task 8/9), `fifa96_camera_save_tail` (Task 9), `goal_counter_15781c` (Task 3), `tracker_15759d` (Task 9). The six post-arm sites are engine-owned; the row-1C and case-6 sites are engine-owned.

**Review Focus:** the eight lines are covered by Task 3 (counter), Task 2 (width/entity), Task 4 (reposition arms), Tasks 5/9 (frame order), Task 6 (jitter), Task 7 (six-arm placement), Task 10 (row-1C routes), Task 11 (case-6 entry).

**Proportion:** the plan is a task map with exact tests and interfaces; the body code is left to the implementer with the census and disassembly, and the stat-only subsets are labeled.
