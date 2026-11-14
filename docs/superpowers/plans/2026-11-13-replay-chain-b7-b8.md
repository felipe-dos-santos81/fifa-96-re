# Replay chain B7–B8 implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the remaining recorded-replay legs: the recorder overlay gate, the six-byte queue/band/queue-pop lane, the 20-dword FIFO and drain half, the arm's projection-reset flag, the appearance-byte identity, the `0x20` camera-cycle sources, and the mode-7 request/timed exit.

**Architecture:** The replay ring (`fifa96_match_replay_ring.c`) owns the view/record block and the composer; `fifa96_match_run.c` owns the replay state machine and the run cells. The new producer bodies live in the engine run/ring (FIFO, overlay latch, mode ramp) or in the loader camera (the focus-band classifier). Every native cell that has no engine producer stays a numbered leg; no leg is replaced by a staged zero that changes reachability.

**Tech Stack:** C11, CMake/CTest, ASan/UBSan (`-fno-sanitize-recover=all`), Ghidra MCP (`/FIFA96.EXE`), the null backend for determinism.

**Spec:** `docs/superpowers/specs/2026-11-12-open-items-closure-slice-b-design.md` §B7/§B8, plus `docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md` §7.8/§13. Task 1 records the fresh-read corrections to both.

## Global Constraints

- Work directly on `main`; commit per task (AGENTS.md). Commit types: `feat`, `fix`, `docs(fuNNN)`, `test`, `chore`.
- `make check` must stay green; strict `-Wall -Wextra -Werror`; engine tests under ASan/UBSan with `-fno-sanitize-recover=all`.
- `port/tests/golden/engine/m1-frames.txt` is **immutable**. M2 is compared after every behavioral landing (Tasks 2–9) and moves only per the design §9 per-incident ruling, with a written reason and frame diff.
- No completion-number movement unless a dispatch-row body appears; the replay producers are not rows. If one is found to be a row, recompute the README/ENGINE numbers in the same commit.
- Docs are append-only: quote the superseded line; never rewrite a prior section.
- Every address/byte claim cites a fresh Ghidra read on `/FIFA96.EXE`; no stale `/tmp` image.
- Unsupported selectors `0x16`, `0x60`, `0x62`, `0x66`, `0x72`, `0x7A` stay UNSUPPORTED (FU-28).
- A producer with no engine cell is a numbered leg, never a fabricated value. A staged-zero input may not make a native branch reachable that the real producer cannot reach.
- The four non-Slice-B `0x8ED40` sites (`0x775FD`/`0x77613`, `0x7ADD2`/`0x7AE04`) stay carried.
- Out of this plan: RR-L1 (embedded ring by design), RR-L6-R (the `0x102`/`0x35C` tick reconciliation), RR-L8 (documented phase mapping), and RR-L9 (the `0x15887B` ball-frame producer, B10).

## Review Focus

- **The recorder gate and reset.** `[0x14E574]` latches the overlay entry and clears `[0x109A90]`; the state-5→6 edge sets it again. `FUN_00064030` does **not** touch `[0x109A90]`, so a ring reset must not silently re-enable recording.
- **Queue lane reachability.** The queue6 native lane starts at `0xFF` and carries object/team indices, not zero. The band/flag/queue-pop are three separate bytes (`0x155C50`, `0x155C51`, `0x155C4F`) with separate producers; the FIFO pop returns `value+1`.
- **FIFO wrap/eviction.** The 20-dword base is `0x155C90..0x155CDC`; index 0 of `0x155C8C` is never written (HUD state). Push pre-increments the head and wraps after the store; pop reads the tail and returns `value+1`.
- **Drain gate.** `FUN_00064EC0` pushes only when `([0x109A98] & 0x80) == 0` and the screen id `[0x105FFC]` is not 5/7. The `0x65CC0` voice notify is a leg.
- **Request chain.** The request source is `FUN_00051270`/`FUN_000512B0` → `FUN_000510DC` → `FUN_00064270`; `FUN_0004B100` is the live frame and is **not** the request source. The `0x14E58C` ramp runs `1 → 0xF0 → 0x168` and exits through `FUN_00064E8C`.
- **Appearance identity.** `**(byte **)(rec+0x28)` and `*[0x158866]` are row pointers; the appearance byte is `row[0]`, so the port's composed `stage.anim_id` is the identity, not a new mapping.
- **Camera identity is partial.** `[0x7DC8]` points into a seven-object pool; the port models the three VIEW classes and ACTION. POSE is fed from the recorded matrix; BALL has no dedicated camera source in the port (record `RR-L5-BALL` if it stays unported). The identity proof covers the six-field shape and the four modeled objects, not all seven.

---

### Task 1: The B7–B8 evidence census and errata

**Files:**
- Create: `docs/superpowers/specs/2026-11-13-replay-chain-census.md`
- Modify: `port/spec/FU108_replay_ring_buffer.md` (append a "Replay-chain census (2026-11-13)" section)
- Modify: `port/spec/FU111_replay_arming_queue_hud.md` (append the FIFO/queue erratum)
- Modify: `port/spec/FU-351_class3_camera.md` (append the C4R-L1 projection-reset correction)
- Modify: `docs/superpowers/specs/2026-11-12-open-items-closure-slice-b-design.md` (append the §11 replay-chain corrections)
- Modify: `docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md` (append the §14 replay-chain corrections)
- Test: none (docs-only; verification is the fixed-address check)

**Interfaces:**
- Produces: the leg table consumed by Tasks 2–9: `leg | native body/address | exact gate/store | engine cell or OPEN | test`.
- Produces: the errata list: RR-L2 ownership (35 xrefs, not owned by `FUN_00053DC4`), `FUN_00064030` does not write `[0x109A90]`, RR-L3's three separate bytes and the queue6 native `0xFF` init, RR-L7's correct request chain, C4R-L1 is presentation-only except `[0x109014]`, RR-L5's partial pool identity, the conditional timed exit, and the `FUN_0006428C` resync.

- [ ] **Step 1: Fresh-read the legs**

Run these Ghidra MCP reads on `/FIFA96.EXE` and record each in the census:
`disassemble_function 0x53DC4`, `disassemble_function 0x53A48`, `disassemble_bytes 0x64070` (the gate stores), `disassemble_function 0x64EF0`, `disassemble_function 0x64EC0`, `disassemble_function 0x974DC`, `disassemble_function 0x65CC0`, `disassemble_function 0x36C70` (the queue6/band/pop block), `disassemble_function 0x627F4` (the band classifier), `disassemble_function 0x4D134`, `disassemble_function 0x4C904`, `disassemble_function 0x510DC`, `disassemble_function 0x51270`, `disassemble_function 0x512B0`, `disassemble_function 0x64270`, `disassemble_function 0x64E74`, `disassemble_function 0x64DFC`, `disassemble_function 0x6428C`, `disassemble_function 0x64E8C`, `disassemble_function 0x5865C`, and `get_xrefs_to 0x14E574` (the 35-xref claim).
Do not use the decompiler as the porting authority where it prunes blocks.

- [ ] **Step 2: Write the leg table**

Create `docs/superpowers/specs/2026-11-13-replay-chain-census.md` with one row per leg. Fix the design's wording in the row:
- **RR-L2:** `0x53DC5 CMP [0x14E574],0; 0x53DD3 CALL 0x64074; 0x53DD8 [0x14E574]=1`; `0x64077 [0x109A90]=0`; `0x64080 [0x109A90]=1`; `0x53A8E state[0x14E578]=6; 0x53A9D CALL 0x64080; 0x53AA2 [0x14E574]=0`. The latch has 35 xrefs across eight functions; the `FUN_00053DC4`/`FUN_00053A48` pair is the recorder disable/re-enable edge, not the latch owner.
- **RR-L3:** queue6 base `0x155C6A..0x155C6F` (`0x109A88` points at it); composer `0x36E79`/`0x36E7F` init `0xFF`, `0x36EB7` stores `[obj+0x8D]`, `0x36ECE` adds `0x0B`, `0x36EE5`/`0x36EF9` store `FUN_0004BA44`/`FUN_0004BA00`; band `0x155C50` from `FUN_000627F4(focus_x, focus_y)`; flag `0x155C51=1`; queue-pop `0x155C4F = FUN_00064EE4()`.
- **RR-L10:** FIFO `0x155C90..0x155CDC`; push pre-increments the head (`0x64F1F`), stores at `0x155C8C + head*4`, wraps after the store; pop reads the tail and returns `value+1` (`0x64F4C`); count/head/tail `[0x109AEC]/[0x109AF0]/[0x109AF4]`; gate `0x64EC3`/`0x64ECD`/`0x64ED6`; notify `0x65CEE`.
- **C4R-L1:** `0x58666 [0x109014]=0` (projection-active flag); `0x5866C FUN_00056B88(0x2B)`; `0x58671 [0x14E9B8]>=0x80` → IDIV 3 over 23 dwords; `0x586B6 FUN_00056894`. The last three are presentation-only.
- **RR-L4:** `FUN_00036C70` appearance `**(byte**)(rec+0x28)`; ball `0x36E20 MOV ECX,[0x158866]; 0x36E2D MOV CL,[ECX]`. Both are row pointers; the byte is `row[0]`.
- **RR-L5:** `FUN_0004D134` table `0x4D118`: 0→POSE `0x107968`, 1/2/3/6→VIEW `0x107CE8`, 4→ACTION `0x107B98`, 5→BALL `0x107B28`; six fields `[0x7DC8]+0x10/14/18/58/5C/4C`; `FUN_0004C904` copies them to `0x14E4E0..0x14E4F4`. The port models VIEW/ACTION; POSE/BALL are fed elsewhere.
- **RR-L7:** the request chain is `FUN_00051270`/`FUN_000512B0` → `FUN_000510DC` → `FUN_00064270`; `FUN_0004B100` is not the request source. `FUN_000510DC(param_1!=0)` sets `[0x14E58C]=1`, calls `FUN_00064270`, `FUN_00037114(4)`, sets `[0x108E0C]=1` and `[0x108E10]=0`. The `param_1==0` arm drives `[0x108E0C]/[0x108E10]` and ramps `[0x14E58C]`; `FUN_00064E8C` is the exit.
- **Timed exit:** `0x511D5 CALL 0x64E74; 0x511DC JNZ 0x51222` gates the exit countdown; `FUN_00064E74` returns `cursor >= count`; `FUN_00064DFC` sets state `0x83` and advances playback while records remain. The ramp alone does not exit.
- **RR-L10 resync:** `0x6428C MOV EAX,[0x155C4C]; 0x64291 SAR EAX,0x18` reads the queue-pop byte `0x155C4F`; a nonzero value is decremented and passed to `FUN_000974DC` (`0x64299`), which calls the `FUN_00064EC0` gate and `FUN_00064EF0` push, then `FUN_00065CC0`.

- [ ] **Step 3: Append the errata**

Append the errata above to FU108/FU111/FU-351 and to the two binding designs (Slice B §11 and ring §14) with quoted superseded lines. Do not rewrite the prior sections.

- [ ] **Step 4: Verify and commit**

Run `grep -Ec '^\| (RR-|C4R-)' docs/superpowers/specs/2026-11-13-replay-chain-census.md` (expect 7 rows) and `git diff --check`.
Expected: 7 leg rows, no whitespace errors.

```bash
git add docs/superpowers/specs/2026-11-13-replay-chain-census.md port/spec/FU108_replay_ring_buffer.md port/spec/FU111_replay_arming_queue_hud.md port/spec/FU-351_class3_camera.md docs/superpowers/specs/2026-11-12-open-items-closure-slice-b-design.md docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md
git commit -m "docs: census the replay chain B7-B8 legs"
```

---

### Task 2: RR-L2 — the recorder overlay gate

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (add `recorder_latch_14e574`)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`fifa96_match_run_recorder_overlay_disable`/`_reenable`; init/begin/end)
- Modify: `port/src/fifa96_engine/fifa96_match_replay_ring.c` (the reset must not write `enabled`)
- Modify: `port/src/fifa96_engine/fifa96_match_replay_ring.h` (correct the native-reset comment)
- Test: `port/tests/test_engine_replay_ring.c`, `port/tests/test_engine_match_frame.c`

**Interfaces:**
- Produces: `int fifa96_match_run_recorder_overlay_disable(struct fifa96_match_run *mr);` — when `recorder_latch_14e574 == 0`: `fifa96_match_replay_ring_record(mr, 0)`, latch `1`, return `1`; otherwise `0`. This is the `FUN_00053DC4` edge (`0x53DD3 CALL 0x64074`, `0x53DD8` latch 1).
- Produces: `int fifa96_match_run_recorder_overlay_reenable(struct fifa96_match_run *mr);` — when latch `1`: `fifa96_match_replay_ring_record(mr, 1)`, latch `0`, return `1`; otherwise `0`. This is the `FUN_00053A48` state-5→6 edge (`0x53A9D CALL 0x64080`, `0x53AA2` latch 0); it is not a generic "overlay closed" call.
- `fifa96_match_replay_ring_reset` stops writing `enabled`; `fifa96_match_run_init` explicitly calls `ring_record(mr, 0)` and clears the latch; `fifa96_match_run_begin` calls `ring_reset`, clears the latch, then `ring_record(mr, 1)`; the end path calls `ring_record(mr, 0)` before `ring_reset`.
- Consumes: the existing ring gate `mr->replay_ring.enabled` (`[0x109A90]`).

- [ ] **Step 1: Write the failing gate tests**

Add `test_recorder_overlay_gate` (a bare run is enough; use `fifa96_match_run_init` and set `replay_ring.enabled = 1`):
1. `disable` → enabled 0, latch 1, return 1; compose and call `capture(mr, 1)` twice → `replay_ring.count == 0` (the two-tick cadence).
2. Repeat `disable` → return 0, state unchanged.
3. `reenable` → enabled 1, latch 0, return 1; compose and call `capture(mr, 1)` twice → count 1.
4. `ring_reset` → enabled stays 1, latch stays 0.
5. Add `test_ring_init_clears_gate` with a `0xAA`-filled run: `fifa96_match_run_init` → `enabled == 0`, latch 0 (the run is not memset by init).
6. Add `test_frame_recorder_gate` in `test_engine_match_frame.c` using the existing fixture (`make_fixture` and the begin/end pattern at `:161-163`): begin → `enabled == 1`, latch 0; end → `enabled == 0`.

Also add the capture-gate assertion to `test_ring_capture_budget` (or a focused fixture) that a closed gate packs nothing even with `stamp_acc >= 2`.

- [ ] **Step 2: Run it to verify it fails**

Run: `cmake --build build --target test_engine_replay_ring test_engine_match_frame -j"$(nproc)" && ./build/test_engine_replay_ring && ./build/test_engine_match_frame`
Expected: FAIL — the overlay functions are undeclared.

- [ ] **Step 3: Implement the gate**

Add the latch field beside `replay_ring`; implement the two functions; remove the `enabled` write from `fifa96_match_replay_ring_reset` (native `FUN_00064030` writes only the three cursors, the playback index `[0x109A94]` and the accumulator `[0x109AAC]`; it does not write `[0x109A90]` or `[0x107310]`); correct the `fifa96_match_replay_ring.h` comment that attributes budget/gate clearing to the native reset. Explicitly initialize the gate/latch in `fifa96_match_run_init` (the run is not memset) and wire begin/end. Update the two existing assertions at `test_engine_replay_ring.c:251,279` that expect `enabled == 0` after reset: set the gate explicitly before the reset and assert the reset leaves it unchanged. The live overlay call site (the native `0x4B1D7`/`0x4B257` path) stays a numbered leg until the overlay state machine is ported; the two functions are the seam.

- [ ] **Step 4: Run the tests and M2 gate**

Run: `cmake --build build --target test_engine_replay_ring test_engine_match_frame test_engine_m1 test_engine_m2 -j"$(nproc)"`; `./build/test_engine_replay_ring`; `./build/test_engine_match_frame`; `./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt`; `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`.
Expected: PASS; both `cmp` 0. If M2 moves, stop.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_match_replay_ring.c port/src/fifa96_engine/fifa96_match_replay_ring.h port/tests/test_engine_replay_ring.c port/tests/test_engine_match_frame.c
git commit -m "feat(engine): model the recorder overlay gate"
```

---

### Task 3: RR-L10 — the 20-dword FIFO and the drain gate

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (the FIFO struct/fields; declarations)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (push/pop/reset/drain)
- Test: `port/tests/test_engine_replay_ring.c`

**Interfaces:**
- Produces:
```c
struct fifa96_match_replay_fifo {
  uint32_t value[20];
  uint32_t count, head, tail;   /* head = next write, tail = oldest, 0..19 */
};
void fifa96_match_run_replay_fifo_reset(struct fifa96_match_run *mr);
int fifa96_match_run_replay_fifo_push(struct fifa96_match_run *mr, uint32_t value);
int fifa96_match_run_replay_fifo_pop(struct fifa96_match_run *mr, uint32_t *value);
int fifa96_match_run_replay_fifo_drain(struct fifa96_match_run *mr, uint32_t value);
```
- Push: if `count == 20`, advance `tail` (wrap at 20) and decrement `count`; store `value[head]`; `head = (head + 1) % 20`; `count++`; return 1. Pop: 0 on empty; else write `value[tail]`, advance `tail` (wrap), `count--`, return 1. This preserves the native observable push/pop order/eviction; the native's pre-increment store lands index 1..20 of `0x155C8C`, while the C ring's index 0 is `0x155C90` — the plan's test pins the order, not the raw address offset.
- Drain pushes only when `(mr->render.replay.state & 0x80) == 0` and `mr->replay_screen_id_105ffc` is not 5/7; the `[0x10A254]` voice notify is a numbered leg.
- Adds `uint8_t replay_screen_id_105ffc;` (staged; the live `[0x105FFC]` producer is a leg) and `struct fifa96_match_replay_fifo replay_fifo;`.

- [ ] **Step 1: Write the failing FIFO tests**

Add `test_replay_fifo_order`: push `1..20`; assert count 20, head 0, tail 0, pop order `1..20`, empty pop returns 0.
Add `test_replay_fifo_eviction`: push `21`; count 20, head 1, tail 1, pop order `2..21`.
Add `test_replay_fifo_wrap`: push 20, pop 5, push 6..10; assert the head/tail wrap and pop order `6..20, 6..10` (the exact sequence after the wrap).
Add `test_replay_fifo_drain_gate`: state `0x80` → drain does not push; state `0` and screen id 5 or 7 → no push; state 0 and screen 0 → push.

- [ ] **Step 2: Run them to verify they fail**

Run: `cmake --build build --target test_engine_replay_ring -j"$(nproc)" && ./build/test_engine_replay_ring`. Expected: FAIL — the FIFO API is undeclared.

- [ ] **Step 3: Implement the FIFO and drain**

Implement the conventional C ring from the Interfaces block: store at `value[head]`, then `head = (head + 1) % 20`; pop from `value[tail]`, then `tail = (tail + 1) % 20`; evict the tail when full. **Do not apply the native address arithmetic literally** — the native pre-increments its head and stores at `0x155C8C + head*4` (index 1..20), while its pop base is `0x155C90` (index 0..19); the C ring preserves the observable order/eviction, not that offset. The C pop returns the raw value with a 1/0 status; the composer adds 1 for the native byte. Reset count/head/tail in `fifa96_match_run_init`/`_begin`, and zero `replay_screen_id_105ffc` there (the live `[0x105FFC]` producer stays a leg). Drain gate as above; the notify is a leg comment.

- [ ] **Step 4: Run the tests and M2 gate**

Run: `cmake --build build --target test_engine_replay_ring test_engine_m1 test_engine_m2 -j"$(nproc)"`; `./build/test_engine_replay_ring`; M1/M2 `cmp`. Expected: PASS; both 0. If M2 moves, stop.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_replay_ring.c
git commit -m "feat(engine): port the replay FIFO and drain gate"
```

---

### Task 4: RR-L3 — the focus band, the band flag and the queue-pop byte

**Files:**
- Modify: `port/src/fifa96_loader/fifa96_camera.h`/`.c` (the band classifier)
- Modify: `port/src/fifa96_engine/fifa96_match_replay_ring.c` (compose the three bytes)
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (the `view_flags_155c4e`/`band`/`flag` cells)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`replay_resync`)
- Test: `port/tests/test_camera.c` (classifier), `port/tests/test_engine_replay_ring.c` (composer/resync)

**Interfaces:**
- Produces: `int fifa96_camera_replay_band(int32_t x, int32_t y);` — `x > 0x46` → 1, `x < -0x46` → 4, else 0; `y < 0x6A` → `|8`, `y > 0x35` → `|2`; return `value + 1`.
- The composer writes `view[+0x1AC] = mr->replay_band_155c50`, `view[+0x1AD] = mr->replay_band_flag_155c51`, `view[+0x1AB] = mr->replay_queue_pop_155c4f`, and `view[+0x1AA] = mr->render.ball_roll | (mr->replay_view_flags_155c4e & 0xC0)` — the native `0x155C4E` byte is the shared roll/flag byte, so the bit-0x40 latch must reach both the view `+0x1AA` and record `+0xF2`.
- The band gate: when `mr->goal_counter_15781c != 0` and `(mr->replay_view_flags_155c4e & 0x40) == 0`: set bit 0x40, `band = fifa96_camera_replay_band(focus_x, focus_y)`, `flag = 1`. The focus is the same triple the composer already uses.
- The queue-pop byte is the FIFO pop status/value: `fifa96_match_run_replay_fifo_pop` into a raw value; the byte is `(uint8_t)(value + 1)` (0 when empty), matching `FUN_00064EE4`.
- `replay_resync` (`FUN_0006428C`) is implemented in this task: if `mr->replay_queue_pop_155c4f != 0`, decrement it and call `fifa96_match_run_replay_fifo_drain(mr, decremented_value)`; the `FUN_00065CC0` voice notify remains a leg. This is the per-countdown-step resync called at `fifa96_match_run.c:10319-10321`.

- [ ] **Step 1: Write the failing classifier and composer tests**

In `test_camera.c`, add `test_replay_band_classifier`: `(0x47,0x6A) → 4`, `(0,0) → 9`, `(-0x47,0) → 13`, `(-0x46,0x6A) → 3`, `(0,0x6B) → 3`.
In `test_engine_replay_ring.c`, extend `test_ring_compose` or add `test_ring_compose_band_queue`:
- stage `goal_counter_15781c = 1`, focus `(0x47,0x6A)`, `render.ball_roll = 0x05`, FIFO with raw values `0x2A, 0x30`; compose → `view[+0x1AC] == 4`, `view[+0x1AD] == 1`, `view[+0x1AB] == 0x2B`, `view[+0x1AA] == 0x45` (roll 0x05 | bit 0x40); pack and assert record `+0xF2 == 0x45`.
- second compose with the flag already set → band/flag unchanged (`+0x1AC == 4`, `+0x1AD == 1`), and the queue-pop byte consumes the next FIFO value (`+0x1AB == 0x31`, because `0x30 + 1`); the pop is outside the band gate.
- a third compose with the FIFO empty → `view[+0x1AB] == 0`.
- Add `test_replay_resync_queue_pop`: set `replay_queue_pop_155c4f = 2`, replay state `0x83` (the live call site's family), call the resync path; assert the pop byte becomes 1 and the FIFO count stays 0 (the `0x64EC0` gate blocks the push while state bit 0x80 is set); a second call decrements to 0; a third leaves it 0. The `FUN_00065CC0` notify is the leg; Task 3's `test_replay_fifo_drain_gate` already proves the push path with state 0.

- [ ] **Step 2: Run them to verify they fail**

Run: `cmake --build build --target test_camera test_engine_replay_ring -j"$(nproc)" && ./build/test_camera && ./build/test_engine_replay_ring`. Expected: FAIL — the classifier and the composer bytes are absent/staged 0.

- [ ] **Step 3: Implement the classifier and the composer bytes**

Port the classifier exactly (signed `CMP`/`JLE`/`JGE`; the `+1` is unconditional). Add the composer gate; keep the `0x155C4E` bit-0x40 latch in `replay_view_flags_155c4e` so it persists across frames, and merge the high two bits into `view[+0x1AA]` as above; zero `replay_view_flags_155c4e`/`replay_band_155c50`/`replay_band_flag_155c51`/`replay_queue_pop_155c4f` in init/begin. Implement `replay_resync` with the decrement+drain; the `0x155C4E` bit 0x80/0x7F arm and the velocity checks after `0x36F36` are legs; record them.

- [ ] **Step 4: Run the tests and M2 gate**

Run: `cmake --build build --target test_camera test_engine_replay_ring test_engine_m1 test_engine_m2 -j"$(nproc)"`; `./build/test_camera`; `./build/test_engine_replay_ring`; M1/M2 `cmp`. Expected: PASS; both 0. If M2 moves, stop.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_loader/fifa96_camera.h port/src/fifa96_loader/fifa96_camera.c port/src/fifa96_engine/fifa96_match_replay_ring.c port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_camera.c port/tests/test_engine_replay_ring.c
git commit -m "feat(engine): compose the replay band and queue-pop bytes"
```

---

### Task 5: RR-L3 — the six-byte queue lane

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (the six-byte queue input)
- Modify: `port/src/fifa96_engine/fifa96_match_replay_ring.c` (compose the `0x1C6..0x1CB` lane)
- Test: `port/tests/test_engine_replay_ring.c`

**Interfaces:**
- Adds `uint8_t replay_queue6[6];` to the run; default `0xFF` at init/begin. The live producer (`0x157C64` objects, `0x157C83`/`0x157C86`, `FUN_0004BA44`/`FUN_0004BA00`) is a numbered leg; the engine stages the lane until that producer is ported. The composer copies `replay_queue6` to `view[+0x1C6..+0x1CB]`.
- Round-trip: the packer already copies the lane to record `+0x10E..+0x113`; the test asserts the record lane and the applied view lane.

- [ ] **Step 1: Write the failing lane test**

Add `test_ring_compose_queue6`: stage `replay_queue6 = {0x11,0x22,0x33,0x44,0x55,0x66}`; compose; assert `view[+0x1C6..+0x1CB]` equals the six bytes; pack with stamp `0x1234`; assert record `+0x10E..+0x113`; unpack into a cleared view; assert the lane returns. A fresh run without staging must compose `0xFF` six times.

- [ ] **Step 2: Run it to verify it fails**

Run: `cmake --build build --target test_engine_replay_ring -j"$(nproc)" && ./build/test_engine_replay_ring`. Expected: FAIL — the lane is `memset 0`.

- [ ] **Step 3: Implement the lane**

Add the field; initialize to `0xFF` in init/begin; replace the `memset(view + QUEUE6, 0, 6)` with the copy. Update the existing `test_ring_compose` assertions at `test_engine_replay_ring.c:365-370` (and their comment) from all-zero to `0xFF` — otherwise the new default fails an old test. The native object-array producer stays a leg; the test stages the lane.

- [ ] **Step 4: Run the tests and M2 gate**

Run: `cmake --build build --target test_engine_replay_ring test_engine_m1 test_engine_m2 -j"$(nproc)"`; `./build/test_engine_replay_ring`; M1/M2 `cmp`. Expected: PASS; both 0. If M2 moves, stop.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_match_replay_ring.c port/tests/test_engine_replay_ring.c
git commit -m "feat(engine): compose the six-byte replay queue lane"
```

---

### Task 6: C4R-L1 — the arm's projection-reset flag

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (the flag)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`replay_begin` sets it 0; init/begin reset)
- Test: `port/tests/test_engine_replay_ring.c`

**Interfaces:**
- Adds `uint8_t replay_view_reset_109014;` (native `[0x109014]`). `fifa96_match_run_replay_begin` sets it `0`; init sets `0` (the native image default; `FUN_000585E4` is the separate setup writer that sets 1, and is a leg); exit leaves it `0`.
- The three presentation calls (`FUN_00056B88(0x2B)`, the `0x14E9B8 >= 0x80` /3 loop, `FUN_00056894`) are numbered legs; the flag is the modelable state.

- [ ] **Step 1: Write the failing flag test**

Add `test_replay_arm_view_reset` using a run staged `running = 1`, `paused = 0`, `lc.screen = FIFA96_MATCH_SCREEN_ACTIVE`: set `replay_view_reset_109014 = 1`; call `fifa96_match_run_replay_begin`; assert the flag is 0 and the state is ARMED. Call `fifa96_match_run_replay_exit`; assert the flag stays 0. A fresh init asserts 0.

- [ ] **Step 2: Run it to verify it fails**

Run: `cmake --build build --target test_engine_replay_ring -j"$(nproc)" && ./build/test_engine_replay_ring`. Expected: FAIL — the field does not exist.

- [ ] **Step 3: Implement the flag**

Add the field; set 0 in init and `replay_begin`; document the three legs and the `FUN_000585E4` setup writer in the commit/report.

- [ ] **Step 4: Run the test and M2 gate**

Run: `cmake --build build --target test_engine_replay_ring test_engine_m1 test_engine_m2 -j"$(nproc)"`; `./build/test_engine_replay_ring`; M1/M2 `cmp`. Expected: PASS; both 0. If M2 moves, stop.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_replay_ring.c
git commit -m "feat(engine): reset the replay projection flag at the arm"
```

---

### Task 7: RR-L4 — the appearance-byte identity proof

**Files:**
- Test: `port/tests/test_engine_replay_ring.c`, `port/tests/test_engine_match_frame.c`
- Modify: `port/src/fifa96_engine/fifa96_match_replay_ring.h` (document the identity; no behavior change)

**Interfaces:**
- Consumes: the existing source-to-stage link (`ball->stage.anim_id = mr->entities.ball.appear_row` at `fifa96_match_run.c:2783-2791`), the composer `view[+0x17C+i] = stage.anim_id`, and the ball slot 22.
- Produces: the proof tests; no composer code change expected. If a test fails, the link/composer is wrong and the task fixes it.

- [ ] **Step 1: Write the identity tests**

In `test_engine_match_frame.c`, add `test_replay_ball_appearance_link`: stage `entities.ball.appear_row = 0x2B` and run a frame with `render.enabled`; assert `render.entities[22].stage.anim_id == 0x2B` (this exercises the existing ball link, not a separately staged copy).
In `test_engine_replay_ring.c`, add `test_ring_appearance_identity`:
1. Stage `render.entities[0..21].stage.anim_id = i + 0x20` and `render.entities[22].stage.anim_id = 0x2B` (the value the link test proved the real stage path produces).
2. Compose; assert `view[+0x17C+i] == stage.anim_id` for every slot (identity, no transform).
3. Pack/unpack; assert the record `+0xC4+i` and the applied lane match.
4. Assert the ball's record byte is the row id (`0x2B`), not a composed row+offset.

- [ ] **Step 2: Run them to verify they pass or expose a defect**

Run: `cmake --build build --target test_engine_replay_ring test_engine_match_frame -j"$(nproc)"`; `./build/test_engine_replay_ring`; `./build/test_engine_match_frame`. Expected: PASS on the current source-to-stage link and composer. If a test fails, fix the link/composer, rebuild `test_engine_m1`/`test_engine_m2`, compare both goldens and stop if either moves.

- [ ] **Step 3: Commit**

```bash
git add port/tests/test_engine_replay_ring.c port/tests/test_engine_match_frame.c port/src/fifa96_engine/fifa96_match_replay_ring.h
git commit -m "test(engine): pin the replay appearance-byte identity"
```

---

### Task 8: RR-L5 — the `0x20` camera cycle and source consumption

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (the source enum/declaration)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (the source selector and the frozen-frame use)
- Test: `port/tests/test_engine_replay_ring.c`, `port/tests/test_engine_match_frame.c`

**Interfaces:**
- Produces: `int fifa96_match_run_replay_camera_source(struct fifa96_match_run *mr);` — returns `FIFA96_MATCH_REPLAY_SOURCE_POSE = 0`, `_VIEW = 1`, `_ACTION = 2`, `_BALL = 3` from the index/kind/sub:
  - index 0 → POSE (the recorded matrix `+0x1AE..0x1C5` wins).
  - indices 1/2/3/6 → VIEW with `camera_sub` 5/6/7/8 → `native_view_cam[camera_sub - 5]` when `native_view_valid` holds, else the live follow source. **The current `native_view_cam[3]`/`native_view_valid[3]` arrays are too small for sub 8 (index 6): widen both to 4 and update the `native_view_class < 3` guards (including `fifa96_match_replay_ring.c:275`) to `< 4`. Class 3 is the sub-8 variant; do not alias it to class 2 and do not read past the array.**
  - index 4 → ACTION → `action_cam`.
  - index 5 → BALL. If the engine has no dedicated ball camera source, return `_BALL` and record the `RR-L5-BALL` leg; the frozen frame keeps the existing live follow.
- The frozen frame uses the source: POSE calls `fifa96_match_replay_pose`; VIEW/ACTION use their handler-state source; BALL uses the ball source or the documented leg.
- The partial identity is recorded: VIEW/ACTION match the four modeled `[0x7DC8]` objects; POSE/BALL have no separate handler state.

- [ ] **Step 1: Write the failing source tests**

Add `test_replay_camera_cycle_sources`:
1. For index 0..6, call `fifa96_match_run_replay_camera_set` and assert `{kind, sub}` and `record_index` match the native table; index 6 selects VIEW sub 8; index > 6 stores `record_index` but keeps the current selection.
2. Call `fifa96_match_run_replay_camera_source` after each set: 0→POSE; 1/2/3/6→VIEW; 4→ACTION; 5→BALL (or the documented BALL leg).
3. The `0x20` button in `fifa96_match_run_replay_step` cycles `5→0` and selects index 0.
4. A POSE replay applies the recorded matrix `+0x1AE..+0x1C5` to `render_eye_*`/`yaw`/`pitch`/`view_ratio`; a VIEW/ACTION source uses its handler state, not the recorded matrix.
5. Index 6/sub 8 uses `native_view_cam[3]`/`native_view_valid[3]` (the widened fourth entry); a `native_view_valid[3] == 0` case falls back to the live follow and does not read past the array.
6. Record the partial-identity/BALL-leg note in the test comment.

- [ ] **Step 2: Run them to verify they fail**

Run: `cmake --build build --target test_engine_replay_ring test_engine_match_frame -j"$(nproc)"`; both targets. Expected: FAIL — the source function is undeclared.

- [ ] **Step 3: Implement the source selector and wire the frozen frame**

Implement the selector from the native map; wire the frozen frame's camera branch. Widen `native_view_cam`/`native_view_valid` to 4 and update the `< 3` guards so sub 8 maps to class 3 without an out-of-bounds read. If the BALL source is absent, keep the existing follow and record `RR-L5-BALL` rather than inventing a source. If code changes, run M1/M2 and stop if either moves.

- [ ] **Step 4: Run the tests and M2 gate**

Run: `cmake --build build --target test_engine_replay_ring test_engine_match_frame test_engine_m1 test_engine_m2 -j"$(nproc)"`; the two tests; M1/M2 `cmp`. Expected: PASS; both 0. If M2 moves, stop.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_replay_ring.c port/tests/test_engine_match_frame.c
git commit -m "feat(engine): consume the replay camera cycle sources"
```

---

### Task 9: RR-L7 — the mode-7 request and timed exit

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (the request/control declarations; the ramp cells)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`replay_request`, `replay_control`)
- Modify: `port/src/fifa96_engine/fifa96_engine.c` (the host replay key uses the request; per-frame control)
- Test: `port/tests/test_engine_replay_ring.c`, `port/tests/test_engine_match_bridge.c`

**Interfaces:**
- Produces:
```c
int fifa96_match_run_replay_request(struct fifa96_match_run *mr, int on);
int fifa96_match_run_replay_control(struct fifa96_match_run *mr, int32_t tick);
int fifa96_match_run_replay_complete(const struct fifa96_match_run *mr);
```
- `request(on=1)`: call `fifa96_match_run_replay_begin(mr)` **first**; only on success set `phase_counter = 1`, `replay_mode_108e0c = 1`, `replay_mode_acc_108e10 = 0`. A NULL/invalid/non-ACTIVE/paused request returns the begin error and leaves all request cells unchanged (the begin already zeroes the phase; do not set it before the call).
- `request(on=0)`: no-op (the per-frame path is `control`).
- `complete(mr)`: `1` when `mr->render.replay.cursor >= mr->replay_ring.count` (the `FUN_00064E74` body; an empty ring is complete), `0` on NULL.
- `control(tick)`: port `FUN_000510DC`'s `param_1==0` arm exactly as the fresh disassembly reads it: `0x510F9..0x5114A` handles the `[0x108E0C]==1` arm (`FUN_000642FC(0)` + `FUN_0004CD88`, then `[0x108E10] += tick`, `[0x108E0C] += 1`); `0x5118B..0x511B4` advances the mode while `[0x108E10] < 0xF0`; `0x511B5..0x51221` clears a non-positive mode and runs the phase ramp (`> 0xF0` → `FUN_00064DFC` leg; `< 0x168` → `+= tick`; else `= 0x169`); `0x511D5 CALL 0x64E74; 0x511DC JNZ 0x51222` gates the exit countdown on `complete`; `0x51222..0x51249` is the countdown (`tick >= phase` → `FUN_00064E8C`, phase `0`, return 1; else phase `-= tick`, return 0).
  **Do not call `fifa96_match_run_replay_step` from `control`.** The frozen-frame branch already calls it once per granted frame; calling it again would double-step. The native `FUN_000642FC(0)`/`FUN_0004CD88` calls in the mode arm and `FUN_00064DFC` are therefore legs mapped to the existing `replay_step` PLAYING path, and the existing frame call remains the state machine. The `0x510EB..0x51119` tick-floor/LIVE early-return arms and `FUN_00045069`'s negative-mode arm are legs/guards; add them to the closure.
- The native arm gate `[0x109AB0] != 0` is the embedded ring's existence (always true); the native `FUN_0005865C` call is Task 6's flag.
- Adds `int32_t replay_mode_108e0c;` and `int32_t replay_mode_acc_108e10;`.

- [ ] **Step 1: Write the failing request/ramp tests**

Add `test_replay_request_timed_exit`:
1. Request on a live, unpaused, ACTIVE run → state ARMED, `phase_counter == 1`, mode 1.
2. Stage a non-empty ring and state PLAYING. Loop at most `0x400` iterations: call `fifa96_match_run_replay_step(mr, 0)` (the existing state machine advances the cursor) then `control(1)`. Assert `control` returns 1 exactly once, the state becomes LIVE, `phase_counter == 0`; record the call index where the phase first reaches `0xF0` and `0x168`; assert the exit happens only after `complete(mr)` returns 1, and that an empty ring can exit after the mode window without a cursor advance.
3. Add `test_replay_request_failure_atomic`: request on a non-ACTIVE/paused run returns `-FIFA96_ERR_STATE` and leaves `phase_counter`/mode/acc unchanged; a NULL request returns `-FIFA96_ERR_INVALID`.
4. The host replay key in `fifa96_engine.c` (the `0x415` `replay_begin` call) changes to `request(mr, 1)`; the frozen-frame branch in `fifa96_match_run.c` (the `replay_family` arm at `:8601`) calls `control(mr, 1)` once per granted frame before `fifa96_match_run_replay_step`.

- [ ] **Step 2: Run them to verify they fail**

Run: `cmake --build build --target test_engine_replay_ring test_engine_match_bridge -j"$(nproc)"`. Expected: FAIL — the request/control/complete API is undeclared.

- [ ] **Step 3: Implement the request and control**

Port the `FUN_000510DC` arm/ramp exactly; port `FUN_00064E74` as `complete`; keep `FUN_00064DFC`/`FUN_00037114`/`FUN_0004CD88` and the `0x510EB..0x51119` arms as legs mapped to the existing state machine/guards. Zero `replay_mode_108e0c`/`replay_mode_acc_108e10` in init/begin. Wire the host key and the per-frame control; do not change the existing `replay_step` state machine and do not call it from `control`.

- [ ] **Step 4: Run the tests and M2 gate**

Run: `cmake --build build --target test_engine_replay_ring test_engine_match_bridge test_engine_match_frame test_engine_m1 test_engine_m2 -j"$(nproc)"`; the three tests; M1/M2 `cmp`; smoke. Expected: PASS; both 0; smoke 124. If M2 moves, stop.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_engine.c port/tests/test_engine_replay_ring.c port/tests/test_engine_match_bridge.c
git commit -m "feat(engine): wire the replay request and timed exit"
```

---

### Task 10: Closure and gate

**Files:**
- Modify: `port/ENGINE.md`, `docs/PROGRESS.md`
- Modify: `port/spec/FU108_replay_ring_buffer.md`, `port/spec/FU111_replay_arming_queue_hud.md`, `port/spec/FU-351_class3_camera.md`, `port/spec/FU72_ring_stats.md`
- Test: the whole suite

**Interfaces:**
- Consumes: Tasks 1–9.

- [ ] **Step 1: Append the closure records**

Record the actual closure per leg and every remaining leg: the `0x157C64` object producer, `FUN_0004BA44`/`FUN_0004BA00`, the `0x65CC0` voice notify, the `FUN_00056B88`/`FUN_00056894` projection bodies, `FUN_000585E4` (the `[0x109014]` setup writer), the `FUN_00051270`/`FUN_000512B0` request sources, the overlay call site/state machine, the `[0x105FFC]` screen-id producer, the `0x155C4E` bit-0x80/velocity producers, the `FUN_00064DFC` playback driver, the `0x510EB..0x51119` tick/LIVE arms, `FUN_00045069`, the BALL camera source (`RR-L5-BALL`), and the `FUN_00064E74`/`FUN_0006428C` status (ported or leg). Quote the superseded lines from FU108/FU111/FU-351 and the design §11/§14 corrections.

- [ ] **Step 2: Recompute the three numbers**

Run `ctest --test-dir build -R test_engine_match_port_census --output-on-failure`. Confirm 78/78 and 61/80/64/80 (or recompute if a row body appeared). Update README/ENGINE only if they moved.

- [ ] **Step 3: Run the full gate**

`make check`; M1 cmp; M2 cmp; `timeout 20s env DISPLAY=:1 make game > /tmp/opencode/replay-chain-smoke.log 2>&1; echo "smoke_exit=$?"`.
Expected: all tests pass; both cmp 0; smoke 124 with a clean log.

- [ ] **Step 4: Commit**

```bash
git add port/ENGINE.md docs/PROGRESS.md port/spec/FU108_replay_ring_buffer.md port/spec/FU111_replay_arming_queue_hud.md port/spec/FU-351_class3_camera.md port/spec/FU72_ring_stats.md
git commit -m "docs: close the replay chain B7-B8"
```

---

## Self-Review

**Spec coverage:** B7's RR-L2 → Task 2; RR-L3 → Tasks 4–5; RR-L10 → Task 3; C4R-L1 → Task 6. B8's RR-L4 → Task 7; RR-L5 → Task 8; RR-L7 → Task 9. B11-style closure → Task 10. The evidence corrections from the fresh reads are Task 1. RR-L1/RR-L6-R/RR-L8/RR-L9 are explicitly out of scope in Global Constraints.

**Step scan:** Every step names a file, a test, a command, or a native address. The exact values are in the tests and interfaces; the bodies are left to the implementer with the native reads.

**Type consistency:** `recorder_latch_14e574`, `replay_fifo`, `replay_screen_id_105ffc`, `replay_queue6`, `replay_view_flags_155c4e`, `replay_band_155c50`, `replay_band_flag_155c51`, `replay_queue_pop_155c4f`, `replay_view_reset_109014`, `replay_mode_108e0c`, `replay_mode_acc_108e10`, `fifa96_camera_replay_band`, the FIFO API, the request/control/complete API, and the replay camera source API are used consistently across tasks.

**Review Focus:** the six lines are covered by Task 2 (gate/reset), Tasks 3–5 (queue/FIFO), Task 3 (drain), Task 9 (request chain), Task 7 (appearance), Task 8 (camera source/partial identity).

**Proportion:** the plan is a task map with exact tests and interfaces; the body code is left to the implementer with the census and native reads.
