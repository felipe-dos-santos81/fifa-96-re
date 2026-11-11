# 23-Descriptor Remap Pool Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the hardcoded kit/skin remap approximation with the native `FUN_00047298` 23-block builder, keeping the port's source-0 render key through a documented adapter.

**Architecture:** A pure `fifa96_match_remap_native` module transcribes the static image tables (`0x106C00` templates, `0x106B70` bands, `0x106B00` source maps, `0x1068E4` map, `0x1070D4`/`0x1070E4` ramps, `0x107080` selector) and builds 23 native 256-byte blocks. The engine copies the 22 visible blocks into `remap_entity` with byte 0 forced to `0xFF` (the renderer's source-0 key), and stores the extra block in `remap_extra`. The extra consumer/provenance and the `FUN_0001CAEC(9/0xD)` option gates stay numbered legs.

**Tech Stack:** C11, CMake/CTest, ASan/UBSan, Ghidra MCP evidence on `/FIFA96.EXE`.

**Spec:** `docs/superpowers/specs/2026-11-11-remap-23-descriptor-pool-design.md` and the campaign design `docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md` §10.

## Global Constraints

- `make check` stays green: strict build (`-Wall -Wextra -Werror`) + full CTest (currently 124/124).
- M1/M2 stay byte-exact; any golden movement is a stop, not a re-pin.
- No completion-number movement (no new body/row; this is a live render internal).
- The builder is native-faithful: its blocks keep byte 0 = `0x00`. The engine's 22 render copies force byte 0 = `0xFF` (the port's source-0 key); the extra block stays native.
- The tables are transcribed into C arrays with the image addresses cited; the engine never reads the EXE at runtime.
- The 23rd block's consumer/provenance and the `FUN_0001CAEC(9/0xD)` gates are numbered legs; stage the gates 0 (normal match).
- Work on `main`; commit per task and push.

## Review Focus

1. **Native byte vs render key** — the builder's block byte 0 is `0x00`; the visible `remap_entity` copy is `0xFF`. Both must be tested; copying the native block unchanged would make every player's background opaque.
2. **23 blocks** — 11 home + 11 away + 1 extra; the extra is stored and stable, never silently dropped.
3. **Table transcription** — 12 bands (not six) and 12 pattern ints per template row; the source maps and the `0x107080` selector are transcribed from the image, not invented.
4. **Substitution rebuild** — the pool follows `sub_lineup`, and only the incoming player's block changes.
5. **Live-only** — M1/M2 never call the builder; the goldens stay byte-exact.

---

### Task 1: The native remap module and table transcription

**Files:**
- Create: `port/src/fifa96_loader/fifa96_match_remap_native.h`
- Create: `port/src/fifa96_loader/fifa96_match_remap_native.c`
- Modify: `port/CMakeLists.txt` (the new loader library and the engine link)
- Test: `port/tests/test_engine_match_render.c`

**Interfaces:**
- Produces:
  - `#define FIFA96_MATCH_REMAP_BLOCKS 23u`
  - `#define FIFA96_MATCH_REMAP_BYTES 256u`
  - `void fifa96_match_remap_build(uint8_t pool[23][256], const uint8_t hair_code[2][11], const uint8_t skin_code[2][11], uint8_t home_blank, uint8_t away_blank);`
  - `uint8_t fifa96_match_remap_hair_class(uint8_t code);`
  - `uint8_t fifa96_match_remap_skin_class(uint8_t code, uint8_t hair_class);`

- [ ] **Step 1: Read the operator and tables from the image**

Use the Ghidra MCP on `/FIFA96.EXE`:
- `decompile_function 0x47298` and `disassemble_function 0x47298` — the 23-block loop, the five template blocks, the pattern pass, the blanking gates.
- `read_memory 0x106C00` (16 rows × `0x30`, 12 pattern ints/row), `0x106B70` (12 `{dest, len, source-group}` entries), `0x106B00` (16 source-map pointers and their 7-entry maps), `0x1068E4` (0x8E map), `0x1070D4` (four 4-byte hair ramps), `0x1070E4` (four skin bases), `0x107080` (five template selectors `0,0,14,15,13`), `0x106AFC..0x106B00` (the template > 0x15 special entries).
- Record the addresses and the table shapes in the C file comments.

- [ ] **Step 2: Write the failing module test**

Add to `port/tests/test_engine_match_render.c` (and include
`"fifa96_loader/fifa96_match_remap_native.h"` with the other loader headers):

```c
/* The native FUN_00047298 builder: 23 blocks, the native byte-0 convention,
 * the hair/skin ramps and the home/away blanking. */
static void test_remap_native_pool(void) {
  uint8_t pool[FIFA96_MATCH_REMAP_BLOCKS][FIFA96_MATCH_REMAP_BYTES];
  uint8_t hair[2][11];
  uint8_t skin[2][11];
  for (int side = 0; side < 2; side++)
    for (int i = 0; i < 11; i++) { hair[side][i] = '4'; skin[side][i] = '4'; }
  /* '4' -> hair class 0 (black) and skin class 0 (base 0xF6). */
  fifa96_match_remap_build(pool, hair, skin, 0, 0);
  assert(pool[0][0] == 0x00);                 /* native byte 0 */
  assert(pool[0][0xD7] == 0x00 && pool[0][0xDA] == 0x00);  /* hair class 0 */
  assert(pool[0][0xDB] == 0xF6 && pool[0][0xE2] == 0xFD);  /* skin base 0 */
  assert(pool[0][0xE3] == 0x00);              /* class 0 clears the last */
  assert(pool[22][0] == 0x00);                /* the extra block is native */

  /* Hair '0' -> class 3 (0xD7..0xDA); skin '1' -> class 3 (base 0xDB). */
  hair[0][0] = '0';
  skin[0][0] = '1';
  fifa96_match_remap_build(pool, hair, skin, 0, 0);
  assert(pool[0][0xD7] == 0xD7 && pool[0][0xDA] == 0xDA);
  assert(pool[0][0xDB] == 0xDB && pool[0][0xE3] == 0xE3);
  assert(pool[1][0xD7] == 0x00);              /* the other players unchanged */

  /* The home gate blanks only the home blocks. */
  fifa96_match_remap_build(pool, hair, skin, 1, 0);
  for (int i = 0xD7; i <= 0xE3; i++) assert(pool[0][i] == 0xFF);
  assert(pool[11][0xD7] == 0x00);             /* away block untouched */
  fifa96_match_remap_build(pool, hair, skin, 0, 1);
  assert(pool[11][0xD7] == 0xFF);
  assert(pool[0][0xD7] == 0xD7);
}
```

Register it in `main()` after `test_replay_target_consumes_stats()` (or the nearest remap test).

- [ ] **Step 3: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: FAIL — `fifa96_match_remap_build` / the constants are undeclared.

- [ ] **Step 4: Create the module and implement the builder**

Create the header with the constants, the four functions and a short contract comment. In the C file:
- Transcribe the tables from Step 1 as `static const` arrays with the image-address comments.
- Implement the five-template composition exactly as `0x47298` does: the `0x107080` selector, the `0x106C00` rows, the `0x106B70` bands, the `0x106B00`/`0x1068E4` maps.
- Implement the per-descriptor output: the 0x7E copy through `0x1068E4`, the hair `0xD7..0xDA`, the skin `0xDB..0xE3` plus the template `+6..+0xE`, the class-0 clear, the home/away blanking, the template-2 second pass, the >0x15 special entries.
- The 23rd descriptor uses template class 3 and the native loop's hair/skin values; its consumer/provenance is a leg.
- `fifa96_match_remap_hair_class`/`skin_class` implement the `0x4C00C`/`0x4C098` maps (`'0'`/`'4'`/else as the existing engine functions).

In `port/CMakeLists.txt`, add:

```cmake
add_library(fifa96_match_remap_native src/fifa96_loader/fifa96_match_remap_native.c)
target_include_directories(fifa96_match_remap_native PUBLIC include PRIVATE src)
```

and add `fifa96_match_remap_native` to the `fifa96_engine` link line.

- [ ] **Step 5: Run the test**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add port/src/fifa96_loader/fifa96_match_remap_native.h port/src/fifa96_loader/fifa96_match_remap_native.c port/CMakeLists.txt port/tests/test_engine_match_render.c
git commit -m "feat(loader): add the native 23-block remap builder"
```

---

### Task 2: Engine integration and the render adapter

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (`remap_extra`)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`match_run_build_entity_remaps`)
- Test: `port/tests/test_engine_match_render.c` (the live substitution test)

**Interfaces:**
- Consumes: Task 1's `fifa96_match_remap_build` and class maps.
- Produces: `uint8_t remap_extra[256];` in `struct fifa96_match_run_render`; the visible remaps built from the native pool with byte 0 = `0xFF`.

- [ ] **Step 1: Write the failing integration assertions**

In `test_live_substitution_rebuilds_remap` (the ISO-gated test that already substitutes roster 5), add:

```c
  /* The visible remap is the native pool with the render key: byte 0 is 0xFF,
   * the native block itself is 0x00. */
  assert(mr->render.remap_entity[0][1][0] == 0xFF);
  assert(mr->render.remap_extra[0] == 0x00);
  /* The native hair/skin bytes for the incoming roster 5 (skin '1' -> class 3,
   * hair '4' -> class 0). */
  assert(mr->render.remap_entity[0][1][0xD7] == 0x00);
  assert(mr->render.remap_entity[0][1][0xDB] == 0xDB);
```

Keep the existing substitution assertions except the old hardcoded band byte
(`remap_entity[0][1][0x75] == 0xDF`) and the old default `0xD7` expectation:
the native template replaces them. Replace those with the native hair/skin
bytes above and one stable template byte the builder produces for the known
roster slot; the adapter and class bytes are the load-bearing assertions.

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: FAIL — `remap_extra` is undeclared (or the native bytes differ).

- [ ] **Step 3: Integrate the builder**

In `fifa96_match_run.h`, add to `struct fifa96_match_run_render`:

```c
  uint8_t remap_extra[256];  /* FUN_00047298's 23rd block (consumer leg) */
```

The builder function gains an explicit `uint8_t remap_extra[256]` output so
the stage path can keep its atomic-local commit:

```c
static void match_run_build_entity_remaps(
    const struct fifa96_match_run *mr,
    const uint8_t remap_team[2][256],
    uint8_t remap_entity[2][FIFA96_MATCH_ENTITY_RECORDS][256],
    uint8_t remap_extra[256]);
```

In `fifa96_match_run.c`, include
`#include "fifa96_loader/fifa96_match_remap_native.h"`.

In `fifa96_match_run.c`, replace `match_run_build_entity_remaps`'s body with:

```c
static void match_run_build_entity_remaps(
    const struct fifa96_match_run *mr,
    const uint8_t remap_team[2][256],
    uint8_t remap_entity[2][FIFA96_MATCH_ENTITY_RECORDS][256],
    uint8_t remap_extra[256]) {
  uint8_t pool[FIFA96_MATCH_REMAP_BLOCKS][FIFA96_MATCH_REMAP_BYTES];
  uint8_t hair[2][11];
  uint8_t skin[2][11];
  (void)remap_team;
  for (int side = 0; side < 2; side++)
    for (int pitch = 0; pitch < (int)FIFA96_MATCH_ENTITY_RECORDS; pitch++) {
      uint8_t roster = mr->sub_lineup[side][pitch];
      hair[side][pitch] = '0';   /* the existing default: hair class 3 */
      skin[side][pitch] = '4';   /* the existing default: skin class 0 */
      if (roster < FIFA96_PCCD_ROSTER_COUNT) {
        hair[side][pitch] = mr->roster_hair_code[side][roster];
        skin[side][pitch] = mr->roster_skin_code[side][roster];
      }
    }
  fifa96_match_remap_build(pool, hair, skin, 0, 0);  /* the option gates are a leg */
  for (int side = 0; side < 2; side++)
    for (int pitch = 0; pitch < (int)FIFA96_MATCH_ENTITY_RECORDS; pitch++) {
      uint8_t *dst = remap_entity[side][pitch];
      memcpy(dst, pool[side * FIFA96_MATCH_ENTITY_RECORDS + pitch], 256);
      dst[0] = 0xFF;   /* the port's source-0 render key (native block is 0) */
    }
  memcpy(remap_extra, pool[22], 256);
}
```

Update the two call sites: `match_run_rebuild_entity_remaps` passes
`mr->render.remap_extra`; `match_run_stage_live_kit` passes a local
`remap_extra_local[256]` and commits it to `r->remap_extra` beside the
`remap_entity` commit after the stage succeeds (preserving the atomic-local
pattern).

Remove the old `match_run_apply_player_remap`, `match_run_remap_band`,
`match_run_hair_ramps`, `match_run_skin_bases`, `match_run_hair_class`,
`match_run_skin_class` and the kit-band constants if they become unused; keep
`remap_team` and the palette path untouched.

- [ ] **Step 4: Run the focused test**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: PASS with the ISO present (the live case runs; without it the case skips).

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_render.c
git commit -m "feat(engine): build the entity remaps from the native 23-block pool"
```

---

### Task 3: Docs, gate and push

**Files:**
- Modify: `port/spec/FU-348_substitution_remap_and_ball_legs.md` (FU348-L2)
- Modify: `port/spec/FU-350_native_lineup_order.md` (FU350-L3)
- Modify: `docs/superpowers/specs/2026-11-11-remap-23-descriptor-pool-design.md` (status)
- Modify: `docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md` (§10 closure)
- Modify: `port/ENGINE.md`, `docs/PROGRESS.md`
- Test: the full gate

**Interfaces:**
- Consumes: Tasks 1–2.
- Produces: the recorded closure and the remaining legs.

- [ ] **Step 1: Update the records**

- FU-348: mark FU348-L2 as closed for the 23-block builder; record the native byte-0 vs render adapter, the table transcription, the extra block leg and the option-gate leg.
- FU-350: mark FU350-L3 as closed for the pool; the 23rd consumer remains.
- The new design doc: a closure section with the landed bytes/adapter/tests.
- Design §10: append a closure record; do not rewrite §10.1-§10.4.
- ENGINE/PROGRESS: phase 117 with the builder, the adapter, the tests and the gate; no completion-number movement.

- [ ] **Step 2: Run the full gate**

Run: `make check` → 124/124.
Run M1/M2 `cmp` → exact.
Run the windowed smoke: `timeout 20s env DISPLAY=:1 make game` → exit 124 with a clean log.

- [ ] **Step 3: Commit and push**

```bash
git add port/spec/FU-348_substitution_remap_and_ball_legs.md port/spec/FU-350_native_lineup_order.md docs/superpowers/specs/2026-11-11-remap-23-descriptor-pool-design.md docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md port/ENGINE.md docs/PROGRESS.md
git commit -m "docs(fu348): record the 23-descriptor remap pool"
git push origin main
```
