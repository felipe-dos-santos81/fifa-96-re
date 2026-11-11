# 23-Descriptor Remap Pool Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the hardcoded kit/skin remap approximation with the native `FUN_00047298` 23-block builder, preserving the port's source-0 render key and the live team-kit overlay.

**Architecture:** A pure `fifa96_match_remap_native` module transcribes the static base template `0x106962`, the 12 `0x106B70` bands, the 28 `0x106B00` source-map pointers into the shared `0x106A1C` region, the `0x106C00` rows 0/13/14/15, the `0x1070D4`/`0x1070E4` ramps and the `0x107080` selector, and builds 23 native 256-byte blocks (11 home, 11 away, one extra class-4 block). The engine computes the native `0x4C00C`/`0x4C098` classes, copies the 22 visible blocks into `remap_entity` with source-0 forced to `0xFF`, applies the existing `remap_team` over the six kit source bands, and stores the extra block in `remap_extra`.

**Tech Stack:** C11, CMake/CTest, ASan/UBSan, Ghidra MCP evidence on `/FIFA96.EXE`.

**Spec:** `docs/superpowers/specs/2026-11-11-remap-23-descriptor-pool-design.md` (including the review errata section) and the campaign design `docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md` §10.

## Global Constraints

- `make check` stays green: strict build (`-Wall -Wextra -Werror`) + full CTest (currently 124/124).
- M1/M2 stay byte-exact; any golden movement is a stop, not a re-pin.
- No completion-number movement (no new body/row; this is a live render internal).
- Native pairing: `class_a` is the `0x4C00C` class (the engine's `match_run_hair_class(roster_hair_code)`); it selects the `0x1070E4` skin base and controls the class-0 `0xE3` clear. `class_b` is the `0x4C098` class (the engine's `match_run_skin_class(roster_skin_code, class_a)`); it selects the `0x1070D4` hair ramp. Do not swap them back.
- The builder composes **five** templates from the static `0x106962` block and builds **23** outputs: home 0 = class 2 (GK), home 1..10 = class 0, away 0 = class 3 (GK), away 1..10 = class 1, extra 22 = class 4 with hardcoded skin/hair class 2/2.
- The visible blocks keep byte 0 = `0xFF`; the native pool blocks keep byte 0 = `0x00`.
- Keep `remap_team` and its existing helpers. Apply the six kit source bands from `remap_team[side]` over each visible native block (all 22, including the GK blocks); do not copy its skin/hair/collar/eye bytes. The GK-specific `FUN_00048DC0` patch and the native group-2 sock/shirt band difference are legs.
- The extra consumer, the `FUN_00048DC0` GK shade patch and the `FUN_0001CAEC(9/0xD)` option gates are numbered legs; stage the gates 0 (normal match).
- Work on `main`; commit per task and push.

## Review Focus

1. **Native class pairing** — `0x4C00C` (port `roster_hair_code`) drives the `0x1070E4` skin base and the `0xE3` clear; `0x4C098` (port `roster_skin_code`) drives the `0x1070D4` hair ramp. The old test values (`0xD7=0`, `0xDB=0xDB` for roster 5) are the non-native pairing and must change.
2. **Team kit overlay** — the native kit bands are default shades (`0xBD/0xBB`), not team destinations. `remap_entity[side][pitch][0x50]` must remain the team slot (`37`/`70`) while `[0x75]` is the native `0x00`.
3. **Five templates / 23 outputs** — home GK, home outfield, away GK, away outfield and the extra must each hit their own template row (classes 2/0/3/1/4); the extra is never dropped.
4. **Table transcription** — 12 bands, 28 map pointers into the 0xE4-byte region, and rows 0/13/14/15; the base+offset map reads (`map[off] + map[off+k+1]`), the in-place band order and the second-pass index 2 must be transcribed, not approximated.
5. **Existing test bytes** — `test_live_substitution_rebuilds_remap`, `test_live_real_team_kits` and `test_live_native_order_appearance` all contain old non-native values; update every one.
6. **Live-only** — M1/M2 never call the builder; the goldens stay byte-exact.

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
  - `void fifa96_match_remap_build(uint8_t pool[23][256], const uint8_t class_a[2][11], const uint8_t class_b[2][11], uint8_t home_blank, uint8_t away_blank);`
    - `class_a[side][pitch]` is the native `FUN_0004C00C` class (0..3; the engine computes it from `roster_hair_code`).
    - `class_b[side][pitch]` is the native `FUN_0004C098` class (0..3; the engine computes it from `roster_skin_code` and `class_a`).
    - The extra descriptor uses `class_a = 2`, `class_b = 2`; its template class is 4.

- [ ] **Step 1: Read the operator and tables from the image**

Use the Ghidra MCP on `/FIFA96.EXE`:
- `disassemble_function 0x47298` — the five-copy template setup at `0x472B8..0x472D3`, the class arrays at `0x473A2..0x473F3`, the extra class at `0x473F5..0x47435`, the composition loop `0x47449..0x47550`, the 23-output loop `0x47578..0x477FE`, the class/table pairing at `0x475DB`/`0x475E2`, the blanking `0x4763A..0x4769C`, the second pass `0x476B8..0x4775C`, and the special guard `0x47762`.
- `decompile_function 0x4C00C` and `0x4C098` — note the `dword[entry+0x1A] >> 24` / `dword[entry+0x1B] >> 24` reads (bytes `entry+0x1D`/`entry+0x1E`); do not mistake the dword base for the byte offset.
- `read_memory`:
  - `0x106962` (0x8E) — the static base template copied five times.
  - `0x106C00` (0x300) — 16 rows of 12 pattern ints; transcribe rows 0, 13, 14, 15 (the selector uses 0/0/14/15/13).
  - `0x106B70` (0x90) — 12 `{dest, len, group}` entries.
  - `0x106B00` (0x70) — 28 map pointers (4 groups × 7 patterns); `0x106A1C` (0xE4) — the shared map region the pointers index. The first pass uses `group*7 + pattern`; the second pass uses `group*7 + 2`. Each map is `[base][offset...]`, and the source is `base + offset`.
  - `0x1068E4` (0x8E) — the output map for the 0x7E copy and the second pass.
  - `0x1070D4` (16) — four 4-byte ramps; class 2 ends `0x43`.
  - `0x1070E4` (0x24) — the four skin bases `F6/ED/E4/DB` (the builder adds 0..8).
  - `0x107080` (0x40) — the five selectors `0,0,14,15,13`.
  - `0x106AFC..0x106AFF` (8) — the special-branch bytes (the guard is unreachable for classes 0..4).
- `get_xrefs_to 0x14BF9C` — the 23rd pool slot has no consumer; record the leg.

- [ ] **Step 2: Write the failing module test**

Add to `port/tests/test_engine_match_render.c` (and include
`"fifa96_loader/fifa96_match_remap_native.h"` with the other loader headers):

```c
/* The native FUN_00047298 builder: five static templates, 23 outputs, the
 * crossed class/table pairing, the extra class-4 block and blanking. */
static void test_remap_native_pool(void) {
  uint8_t pool[FIFA96_MATCH_REMAP_BLOCKS][FIFA96_MATCH_REMAP_BYTES];
  uint8_t class_a[2][11];
  uint8_t class_b[2][11];
  memset(class_a, 0, sizeof class_a);
  memset(class_b, 0, sizeof class_b);

  fifa96_match_remap_build(pool, class_a, class_b, 0, 0);
  assert(pool[0][0] == 0x00);                 /* native byte 0 */
  /* Five template classes: home GK (2), home outfield (0), away GK (3),
   * away outfield (1), extra (4). */
  assert(pool[0][0x50] == 0xC0);              /* home GK, row 14 */
  assert(pool[0][0x52] == 0xC0);
  assert(pool[1][0x50] == 0xBD);              /* home outfield, row 0 */
  assert(pool[1][0x52] == 0xBC);
  /* Sample every band group: 48, 59, 70, 81, 119, 19 (shorts), 26
   * (torso) and the band-101 second pass. */
  assert(pool[1][0x5B] == 0xBD && pool[1][0x5D] == 0xBC);  /* 48 */
  assert(pool[1][0xA5] == 0x00 && pool[1][0xA6] == 0x00);  /* 59 */
  assert(pool[1][0x66] == 0xBD && pool[1][0x68] == 0xBC);  /* 70 */
  assert(pool[1][0x8A] == 0xBD && pool[1][0x8C] == 0xBC);  /* 81 */
  assert(pool[1][0x89] == 0x00);                           /* 119 */
  assert(pool[1][0xCB] == 0x00);                           /* 19 shorts */
  assert(pool[1][0xD4] == 0x00);                           /* 26 torso */
  assert(pool[1][0x75] == 0xFA && pool[1][0x76] == 0xFB);  /* 101 2nd pass */
  assert(pool[1][0xC2] == 0xC0 && pool[1][0xC3] == 0xBE &&
         pool[1][0xC4] == 0xBB);              /* 15 collar/eye band */
  assert(pool[1][0x4E] == 0xBB);
  assert(pool[11][0x50] == 0x00);             /* away GK, row 15 */
  assert(pool[12][0x50] == 0xBD);             /* away outfield, row 0 */
  assert(pool[22][0x00] == 0x00);
  assert(pool[22][0x50] == 0x00 && pool[22][0x51] == 0x00); /* extra row 13 */
  assert(pool[22][0x75] == 0xE8 && pool[22][0x76] == 0xE9); /* extra 2nd pass */
  /* The extra is hardcoded class_a 2 / class_b 2. */
  assert(pool[22][0xD7] == 0xF0 && pool[22][0xDA] == 0x43);
  assert(pool[22][0xDB] == 0xE4 && pool[22][0xE3] == 0xEC);

  /* class_a 0 -> skin F6..FD and the 0xE3 clear;
   * class_b 3 -> hair D7..DA. */
  class_a[0][1] = 0;
  class_b[0][1] = 3;
  fifa96_match_remap_build(pool, class_a, class_b, 0, 0);
  assert(pool[1][0xD7] == 0xD7 && pool[1][0xDA] == 0xDA);
  assert(pool[1][0xDB] == 0xF6 && pool[1][0xE2] == 0xFD);
  assert(pool[1][0xE3] == 0x00);

  /* class_b 2 -> hair F0..F2,0x43; class_a 0 keeps the F6 skin. */
  class_b[0][1] = 2;
  fifa96_match_remap_build(pool, class_a, class_b, 0, 0);
  assert(pool[1][0xD7] == 0xF0 && pool[1][0xDA] == 0x43);
  assert(pool[1][0xDB] == 0xF6 && pool[1][0xE3] == 0x00);

  /* class_a 3 -> skin DB..E3; class_b 0 -> hair 0x00. */
  class_a[0][1] = 3;
  class_b[0][1] = 0;
  fifa96_match_remap_build(pool, class_a, class_b, 0, 0);
  assert(pool[1][0xD7] == 0x00 && pool[1][0xDB] == 0xDB);
  assert(pool[1][0xE3] == 0xE3);

  /* Home gate 9 blanks home 0..10 only. */
  memset(class_a, 0, sizeof class_a);
  memset(class_b, 0, sizeof class_b);
  fifa96_match_remap_build(pool, class_a, class_b, 1, 0);
  for (int i = 0xD7; i <= 0xE3; i++) assert(pool[0][i] == 0xFF);
  assert(pool[11][0xD7] == 0x00);
  /* Away gate 0xD blanks away 11..21 only. */
  fifa96_match_remap_build(pool, class_a, class_b, 0, 1);
  assert(pool[11][0xD7] == 0xFF);
  assert(pool[0][0xD7] == 0x00);
}
```

Register it in `main()` after `test_replay_target_consumes_stats()` (or the nearest remap test).

- [ ] **Step 3: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: FAIL — `fifa96_match_remap_build` / the constants are undeclared.

- [ ] **Step 4: Create the module and implement the builder**

Create the header with the constants, the builder and a contract comment naming
the native class pairing. In the C file:
- Transcribe the tables from Step 1 as `static const` arrays with the
  image-address comments:
  - `remap_base[0x8E]` = `0x106962`.
  - `remap_map8e[0x8E]` = `0x1068E4`.
  - `remap_maps[0xE4]` = `0x106A1C..0x106B00`.
  - `remap_map_ptr[28]` = the relative offsets of the `0x106B00` pointers.
  - `remap_bands[12][3]` = `0x106B70` (`{dest, len, group}`).
  - `remap_rows[4][12]` = image rows 0, 13, 14, 15; `remap_row_for_class[5] = {0, 0, 2, 3, 1}` (local indices).
  - `remap_hair[4][4]` = `0x1070D4`; `remap_skin[4]` = the four bases
    `F6/ED/E4/DB` from `0x1070E4`.
- Compose the five templates from the **same** `remap_base` copy:
  - For class 0..4, `t = remap_base`, `row = remap_rows[remap_row_for_class[class]]`.
  - Process the 12 bands in table order (37, 48, 59, 70, 81, 92, 101, 110,
    119, 15, 19, 26); the in-place reads depend on that order.
  - If `row[j] == 2`: `t[dest] = 0`; else for `k = 0..len-1`:
    `t[dest + k] = t[remap_maps[off] + remap_maps[off + k + 1]]`, where
    `off = remap_map_ptr[group * 7 + row[j]]` (base+offset; the first byte
    is fixed, the following bytes advance).
- Build the 23 outputs:
  - `out = identity 0..255`.
  - `t = templates[template_class(i)]` (copy; the skin writes mutate `t`).
  - For `k = 0..0x7D`: `out[remap_map8e[k]] = t[k]`.
  - `a = (i == 22) ? 2 : class_a[side][pitch]`; `b = (i == 22) ? 2 :
    class_b[side][pitch]`; clamp both to 0..3.
  - Hair: `out[0xD7 + k] = remap_hair[b * 4 + k]` for k 0..3.
  - Skin: `v = remap_skin[a] + k`; `out[0xDB + k] = v`; `t[6 + k] = v` for
    k 0..8.
  - If `a == 0`: `out[0xE3] = 0`; `t[0xE] = 0`.
  - Blanking: if `home_blank && i < 11`, or `away_blank && 11 <= i < 22`,
    set `out[0xD7..0xE3] = 0xFF` and `t[6..0xE] = 0xFF`.
  - Second pass (using the same `row` as the descriptor's template class):
    for each band `j` with `row[j] == 2`, use
    `off1 = remap_map_ptr[group * 7 + 2]`, and for `k = 0..len-1`:
    `out[remap_map8e[dest + k]] = t[remap_maps[off1] + remap_maps[off1 + k + 1]]`.
  - The `0x107080[class] > 0x15` special branch is unreachable for classes
    0..4; leave a comment and record it as a leg (do not test it).
- The class assignment is fixed: `i == 0` -> 2, `1..10` -> 0, `i == 11` ->
  3, `12..21` -> 1, `i == 22` -> 4.
- The new `remap_hair` table is the image's `0x1070D4` (`F0 F1 F2 43` for
  class 2); do not reuse the legacy `match_run_hair_ramps` (`F3`).
- No error returns; the builder is pure data.

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

### Task 2: Engine integration, the render adapter and the team-kit overlay

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (`remap_extra`)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`match_run_build_entity_remaps` and the overlay)
- Test: `port/tests/test_engine_match_render.c` (the live substitution, real-kit and native-order tests)

**Interfaces:**
- Consumes: Task 1's `fifa96_match_remap_build`.
- Produces: `uint8_t remap_extra[256];` in `struct fifa96_match_run_render`; the visible remaps built from the native pool with byte 0 = `0xFF` and the six kit source bands overlaid from `remap_team`.

- [ ] **Step 1: Update the failing integration assertions**

Update all three affected tests. The old non-native values are retired. Keep
the surrounding stage/pause/substitution mechanics; replace only the listed
remap-byte assertions.

`test_live_substitution_rebuilds_remap`:

```c
  /* roster 0: class_a 0 (skin F6..FD, 0xE3 clear), class_b 2
   * (hair F0..F2,0x43). The team kit overlay keeps the shirt slot. */
  assert(r->remap_entity[0][1][0x50] == 37);
  assert(r->remap_entity[0][1][0xD7] == 0xF0);
  assert(r->remap_entity[0][1][0xDA] == 0x43);
  assert(r->remap_entity[0][1][0xDB] == 0xF6);
  assert(r->remap_entity[0][1][0xE3] == 0x00);
  assert(r->remap_entity[0][2][0xD7] == 0x00);   /* unassigned default */
  assert(r->remap_entity[0][2][0xDB] == 0xDB);
  assert(r->remap_entity[0][2][0xE3] == 0xE3);
  ...
  /* roster 5: class_a 0 (skin F6), class_b 3 (hair D7..DA). */
  assert(r->remap_entity[0][1][0x50] == 37);
  assert(r->remap_entity[0][1][0xD7] == 0xD7);
  assert(r->remap_entity[0][1][0xDB] == 0xF6);
  assert(r->remap_entity[0][1][0xE3] == 0x00);
  assert(r->remap_entity[0][2][0xD7] == 0x00);
  assert(r->remap_entity[0][2][0xDB] == 0xDB);
```

`test_live_real_team_kits`:

```c
  /* Native appearance + team-kit overlay. The old
   * memcmp(remap_entity, remap_team) no longer holds: only the six kit
   * source bands come from remap_team; the rest is the native block. */
  assert(r->remap_entity[0][1][0x50] == r->remap_team[0][0x50]);
  assert(r->remap_entity[0][1][0xD7] == 0x00);
  assert(r->remap_entity[0][1][0xDB] == 0xDB);
  assert(r->remap_entity[0][1][0xE3] == 0xE3);
  ...
  /* roster 0 (hair '4' / skin '3'): class_a 0, class_b 2. */
  assert(r->remap_entity[0][1][0xD7] == 0xF0);
  assert(r->remap_entity[0][1][0xDA] == 0x43);
  assert(r->remap_entity[0][1][0xDB] == 0xF6);
  assert(r->remap_entity[0][1][0xE3] == 0x00);
  assert(r->remap_entity[0][2][0xD7] == 0x00);
  assert(r->remap_entity[0][2][0xDB] == 0xDB);
  assert(r->remap_entity[0][2][0xE3] == 0xE3);
  assert(r->remap_entity[1][1][0x50] == 70);   /* away overlay */
```

`test_live_native_order_appearance`:

```c
  /* Native pairing: class_a = 0x4C00C (port hair code), class_b =
   * 0x4C098 (port skin code). */
  assert(r->remap_entity[0][0][0xD7] == 0xEB);   /* skin '2' -> class_b 1 */
  assert(r->remap_entity[0][0][0xDB] == 0xE4);   /* hair '1' -> class_a 2 */
  assert(r->remap_entity[0][1][0xD7] == 0x00);   /* skin default '4' -> class_b 0 */
  assert(r->remap_entity[0][1][0xDB] == 0xED);   /* hair '2' -> class_a 1 */
  assert(r->remap_entity[1][2][0xD7] == 0xD7);
  assert(r->remap_entity[1][2][0xDB] == 0xDB);
  assert(r->remap_entity[1][4][0xD7] == 0xEB);   /* skin '2' -> class_b 1 */
  assert(r->remap_entity[1][4][0xDB] == 0xDB);   /* hair '0' -> class_a 3 */
```

Remove the old `[0x75]` assertions (`0xE8`/`0xDF`/`0xF1`).

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: FAIL — `remap_extra` is undeclared and the old bytes differ.

- [ ] **Step 3: Integrate the builder and the overlay**

In `fifa96_match_run.h`, add to `struct fifa96_match_run_render`:

```c
  uint8_t remap_extra[256];  /* FUN_00047298's 23rd block (consumer leg) */
```

The builder function gains an explicit `uint8_t remap_extra[256]` output so
the stage path keeps its atomic-local commit:

```c
static void match_run_build_entity_remaps(
    const struct fifa96_match_run *mr,
    const uint8_t remap_team[2][256],
    uint8_t remap_entity[2][FIFA96_MATCH_ENTITY_RECORDS][256],
    uint8_t remap_extra[256]);
```

In `fifa96_match_run.c`, include
`#include "fifa96_loader/fifa96_match_remap_native.h"` and replace the
builder body with:

```c
static void match_run_overlay_team_kit(uint8_t dst[256],
                                       const uint8_t team[256]) {
  static const uint8_t bands[][2] = {
      {0x50, 0x65}, {0x8A, 0x9D}, {0xCC, 0xD6},
      {0xC5, 0xCB}, {0x66, 0x70}, {0x9E, 0xA8},
  };
  for (size_t i = 0; i < sizeof bands / sizeof bands[0]; i++)
    memcpy(dst + bands[i][0], team + bands[i][0],
           (size_t)(bands[i][1] - bands[i][0] + 1));
}

static void match_run_build_entity_remaps(
    const struct fifa96_match_run *mr,
    const uint8_t remap_team[2][256],
    uint8_t remap_entity[2][FIFA96_MATCH_ENTITY_RECORDS][256],
    uint8_t remap_extra[256]) {
  uint8_t pool[FIFA96_MATCH_REMAP_BLOCKS][FIFA96_MATCH_REMAP_BYTES];
  uint8_t class_a[2][11];
  uint8_t class_b[2][11];
  for (int side = 0; side < 2; side++)
    for (int pitch = 0; pitch < (int)FIFA96_MATCH_ENTITY_RECORDS; pitch++) {
      uint8_t hair_code = '0';   /* the existing default */
      uint8_t skin_code = '4';
      uint8_t roster = mr->sub_lineup[side][pitch];
      if (roster < FIFA96_PCCD_ROSTER_COUNT) {
        hair_code = mr->roster_hair_code[side][roster];
        skin_code = mr->roster_skin_code[side][roster];
      }
      class_a[side][pitch] = match_run_hair_class(hair_code);
      class_b[side][pitch] =
          match_run_skin_class(skin_code, class_a[side][pitch]);
    }
  fifa96_match_remap_build(pool, class_a, class_b, 0, 0);  /* gates: leg */
  for (int side = 0; side < 2; side++)
    for (int pitch = 0; pitch < (int)FIFA96_MATCH_ENTITY_RECORDS; pitch++) {
      uint8_t *dst = remap_entity[side][pitch];
      memcpy(dst, pool[side * FIFA96_MATCH_ENTITY_RECORDS + pitch], 256);
      match_run_overlay_team_kit(dst, remap_team[side]);
      dst[0] = 0xFF;   /* the port's source-0 render key (native is 0x00) */
    }
  memcpy(remap_extra, pool[22], 256);
}
```

Update the two call sites: `match_run_rebuild_entity_remaps` passes
`mr->render.remap_extra`; `match_run_stage_live_kit` passes a local
`remap_extra_local[256]` and commits it to `r->remap_extra` beside the
`remap_entity` commit after the stage succeeds.

Keep the existing `match_run_apply_player_remap`, `match_run_remap_band`,
`match_run_hair_ramps`, `match_run_skin_bases`, `match_run_hair_class` and
`match_run_skin_class`: `remap_team` still uses them. Do not remove them.
The legacy `match_run_hair_ramps` class-2 `F3` is not used by the native
builder (the native `0x43` lives in the new module); leave it unchanged in
this slice.

- [ ] **Step 4: Run the focused test**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: PASS with the ISO present (the live cases run; without it they skip).

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_render.c
git commit -m "feat(engine): build entity remaps from the native 23-block pool"
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

- FU-348: mark FU348-L2 closed for the five-template/23-block builder; record
  the native class pairing, the extra class 4 / skin-hair class 2/2, the
  team-kit overlay, the extra-consumer leg, the `FUN_00048DC0` GK shade leg,
  the `FUN_0001CAEC(9/0xD)` gate leg and the unreachable special branch.
- FU-350: mark FU350-L3 closed for the pool; the 23rd consumer remains.
- The new design doc: a closure section with the landed bytes/adapter/tests
  (the review errata above already records the corrected evidence).
- Design §10: append a closure record; do not rewrite §10.1–§10.4.
- ENGINE/PROGRESS: phase 117 with the builder, the adapter, the tests and the
  gate; no completion-number movement.

- [ ] **Step 2: Run the full gate**

Run: `make check` → 124/124.
Run M1/M2 `cmp` → exact (stdout-only pipes).
Run the windowed smoke: `timeout 20s env DISPLAY=:1 make game` → exit 124
with a clean log.

- [ ] **Step 3: Commit and push**

```bash
git add port/spec/FU-348_substitution_remap_and_ball_legs.md port/spec/FU-350_native_lineup_order.md docs/superpowers/specs/2026-11-11-remap-23-descriptor-pool-design.md docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md port/ENGINE.md docs/PROGRESS.md
git commit -m "docs(fu348): record the 23-descriptor remap pool"
git push origin main
```
