# FIFA96 Container Decode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Parse the FIFA96 10-byte container envelope in portable C with golden tests, then locate the decode funnel in the EXE and either implement the proven codec or document FU-3.

**Architecture:** Pure `fifa96_envelope` header parser (bytes-in, no I/O) over the existing `fifa96_file` reader; Ghidra INT-21h-anchored funnel pass feeds rename-backflow into `docs/ghidra/loader_rename_map.md` and the 5 decoder header citations; codec is implemented only from a fully-proven algorithm, otherwise FU-3 captures the exact runtime experiment.

**Tech Stack:** C11, gcc 13.3, CMake 3.28, CTest, `xxd` (golden citations), Ghidra-MCP (`decompile_function`, `get_xrefs_*`, `search_instructions`, `rename_function`, `set_comment`, `save_program`), DOSBox-X oracle (FU-3 only).

**Spec:** `docs/superpowers/specs/2026-09-28-fifa96-container-decode-design.md`

## Global Constraints

- C11 only, flags `-Wall -Wextra -Werror`.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`; goldens are copies under `tests/golden/`.
- No byte offset, byte value, or algorithm step enters code without a golden/`xxd`/instruction citation.
- All byte assertions in tests must be derivable by `xxd -s OFF -l N` on the committed golden (cite the command in the test comment).
- The codec is either implemented with a full citation chain, or documented as FU-3 with the exact verification experiment. A half-implementation asserting "this decodes it" without a reference output is a defect.
- Ghidra work targets project `fifa96`, program `/fifa96.exe`, with `save_program` after each rename batch.
- Slice-1 API is frozen: old parsers keep behavior and goldens; only comments may gain envelope pointers.

---

## Scope Check

Spec slice 2 covers one subsystem (container envelope + decode funnel) with three formats sharing one 10-byte prefix. One plan, three tasks. Task 1 is independently testable (`test_envelope`). Task 2 is an evidence pass whose deliverable is the extended rename map + concrete header citations + a block-walk verdict (verified by file content + full-suite regression). Task 3 is conditional (codec impl with vectors, or FU-3 doc). No split into sub-plans needed.

## File Structure

- Create: `include/fifa96_loader/fifa96_envelope.h` — `fifa96_envelope_hdr_t` + `fifa96_envelope_parse_hdr` declaration.
- Create: `src/fifa96_loader/fifa96_envelope.c` — pure header parse using `fifa96_read_u16le`.
- Create: `tests/test_envelope.c` — 3-format header goldens + fw1.qfs `GIMX`@0x12 assert + negative paths.
- Modify: `CMakeLists.txt` — append `fifa96_envelope` lib + `test_envelope` target (existing pattern).
- Modify: `include/fifa96_loader/fifa96_qfs.h` — append envelope pointer comment (no behavior change).
- Modify: `include/fifa96_loader/fifa96_pog.h` — append envelope pointer comment (no behavior change).
- Modify: `docs/ghidra/loader_rename_map.md` — append codec funnel section (Task 2).
- Modify: `include/fifa96_loader/fifa96_{pog,qfs,viv,tgv,tables}.h` — concrete funnel citations (Task 2, comments only).
- Create: `docs/ghidra/FU3_codec_runtime_capture.md` — ONLY on Task 3 FU-3 path.
- Modify: `docs/ghidra/FU1_FU2_closeout.md` — append FU-3 section (Task 3, both paths record the verdict).
- Create (only on Task 3 codec path): `include/fifa96_loader/fifa96_codec.h`, `src/fifa96_loader/fifa96_codec.c`, `tests/test_codec.c`.

---

### Task 1: fifa96_envelope header parser + golden tests + envelope pointer comments

**Files:**
- Create: `include/fifa96_loader/fifa96_envelope.h`
- Create: `src/fifa96_loader/fifa96_envelope.c`
- Create: `tests/test_envelope.c`
- Modify: `CMakeLists.txt` (append only)
- Modify: `include/fifa96_loader/fifa96_qfs.h` (one comment line)
- Modify: `include/fifa96_loader/fifa96_pog.h` (one comment line)

**Interfaces:**
- Consumes: `fifa96_err_t fifa96_file_read(const char *path, uint8_t **out, size_t *out_len);` `void fifa96_file_free(uint8_t *p);` `uint16_t fifa96_read_u16le(const uint8_t *p);` from Task-independent slice-1 `fifa96_file`.
- Produces:
  - `typedef struct { uint16_t magic; uint16_t word_a; uint16_t word_b; char tag[4]; size_t tail_off; size_t tail_len; } fifa96_envelope_hdr_t;`
  - `fifa96_err_t fifa96_envelope_parse_hdr(const uint8_t *b, size_t n, fifa96_envelope_hdr_t *h);` — `FIFA96_OK` on a 10-byte header with magic `0xFB10`; `FIFA96_ERR_TRUNCATED` on NULL buffer/out or `n < 10`; `FIFA96_ERR_BAD_MAGIC` on wrong magic with `n >= 10`. Sets `tail_off = 10`, `tail_len = n - 10`. No block walk, no new error code.

- [ ] **Step 1: Write the failing test**

```c
// tests/test_envelope.c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_envelope.h"
int main(void) {
  uint8_t *q = 0, *p = 0, *g = 0; size_t nq = 0, np = 0, ng = 0;
  fifa96_envelope_hdr_t h;
  /* fw1.qfs: xxd -s 0 -l 24 tests/golden/fw1.qfs */
  assert(fifa96_file_read("tests/golden/fw1.qfs", &q, &nq) == 0);
  assert(nq == 9951);
  assert(fifa96_envelope_parse_hdr(q, nq, &h) == 0);
  assert(h.magic == 0xfb10);                 /* bytes 0-1: 10 fb */
  assert(h.word_a == 0xd400);                /* bytes 2-3: 00 d4 */
  assert(h.word_b == 0xe440);                /* bytes 4-5: 40 e4 */
  assert(memcmp(h.tag, "SHPI", 4) == 0);     /* bytes 6-9: 53 48 50 49 */
  assert(h.tail_off == 10);
  assert(h.tail_len == nq - 10);
  assert(memcmp(q + 0x12, "GIMX", 4) == 0);  /* xxd -s 0x12 -l 4: 47 49 4d 58 */
  fifa96_file_free(q);
  /* pcindex.pog: xxd -s 0 -l 24 tests/golden/pcindex.pog */
  assert(fifa96_file_read("tests/golden/pcindex.pog", &p, &np) == 0);
  assert(np == 25857);
  assert(fifa96_envelope_parse_hdr(p, np, &h) == 0);
  assert(h.magic == 0xfb10);
  assert(h.word_a == 0xb600);                /* bytes 2-3: 00 b6 */
  assert(h.word_b == 0xe140);                /* bytes 4-5: 40 e1 */
  assert(memcmp(h.tag, "PCNX", 4) == 0);     /* bytes 6-9: 50 43 4e 58 */
  fifa96_file_free(p);
  /* gameart0.pvi: xxd -s 0 -l 24 tests/golden/gameart0.pvi */
  assert(fifa96_file_read("tests/golden/gameart0.pvi", &g, &ng) == 0);
  assert(ng == 154387);
  assert(fifa96_envelope_parse_hdr(g, ng, &h) == 0);
  assert(h.magic == 0xfb10);
  assert(h.word_a == 0x1704);                /* bytes 2-3: 04 17 */
  assert(h.word_b == 0xe3a2);                /* bytes 4-5: a2 e3 */
  assert(memcmp(h.tag, "BIGF", 4) == 0);     /* bytes 6-9: 42 49 47 46 */
  fifa96_file_free(g);
  /* negative paths (no I/O) */
  uint8_t z[16] = {0};
  assert(fifa96_envelope_parse_hdr(0, 16, &h) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_envelope_parse_hdr(z, 16, 0) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_envelope_parse_hdr(z, 9, &h) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_envelope_parse_hdr(z, 16, &h) == FIFA96_ERR_BAD_MAGIC);
  printf("test_envelope OK\n");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake -S . -B build && cmake --build build 2>&1 | tail -n 5`
Expected: FAIL — `fifa96_loader/fifa96_envelope.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

```c
// include/fifa96_loader/fifa96_envelope.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { uint16_t magic; uint16_t word_a; uint16_t word_b; char tag[4]; size_t tail_off; size_t tail_len; } fifa96_envelope_hdr_t;
// Ghidra: file I/O via file_open_dos@11bd:5fb8 / file_read_dos@11bd:5fe2 (INT-21h wrappers, verified). The 10-byte envelope (magic 0xFB10 + word_a@2 + word_b@4 + 4cc tag@6, tail@10) is behaviorally reconstructed from the golden CD data — the decode funnel it feeds is mapped in Task 2 (see docs/ghidra/loader_rename_map.md, FU-1/FU-3). See docs/ghidra/FU1_FU2_closeout.md.
fifa96_err_t fifa96_envelope_parse_hdr(const uint8_t *b, size_t n, fifa96_envelope_hdr_t *h);
```

```c
// src/fifa96_loader/fifa96_envelope.c
#include "fifa96_loader/fifa96_envelope.h"
#include "fifa96_loader/fifa96_file.h"
#include <string.h>
fifa96_err_t fifa96_envelope_parse_hdr(const uint8_t *b, size_t n, fifa96_envelope_hdr_t *h) {
  if (!b || !h || n < 10) return FIFA96_ERR_TRUNCATED;
  if (b[0] != 0x10 || b[1] != 0xfb) return FIFA96_ERR_BAD_MAGIC;
  h->magic = fifa96_read_u16le(b);
  h->word_a = fifa96_read_u16le(b + 2);
  h->word_b = fifa96_read_u16le(b + 4);
  memcpy(h->tag, b + 6, 4);
  h->tail_off = 10;
  h->tail_len = n - 10;
  return FIFA96_OK;
}
```

```cmake
# append to CMakeLists.txt
add_library(fifa96_envelope src/fifa96_loader/fifa96_envelope.c)
target_link_libraries(fifa96_envelope PRIVATE fifa96_file)
add_executable(test_envelope tests/test_envelope.c)
target_link_libraries(test_envelope PRIVATE fifa96_envelope fifa96_file)
add_test(NAME test_envelope COMMAND test_envelope WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
```

Envelope pointer comments (append exactly one line each, no behavior change):

`include/fifa96_loader/fifa96_qfs.h` — append after the existing `// Ghidra:` comment line:
```c
// Envelope: the 10-byte uniform prefix (magic/word_a/word_b/tag@6) is parsed by fifa96_envelope_parse_hdr; dec_len (u32 @2) here straddles word_a/word_b and is kept for golden compatibility.
```

`include/fifa96_loader/fifa96_pog.h` — append after the existing `// Ghidra:` comment line:
```c
// Envelope: the 10-byte uniform prefix (magic/word_a/word_b/tag@6) is parsed by fifa96_envelope_parse_hdr; w2 (u32 @4) here overlaps tag@6 and is kept for golden compatibility.
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build -V`
Expected: PASS — `test_envelope OK`, full suite green (9/9: 8 slice-1 + test_envelope).

- [ ] **Step 5: Commit**

```bash
git add include/fifa96_loader/fifa96_envelope.h src/fifa96_loader/fifa96_envelope.c tests/test_envelope.c CMakeLists.txt include/fifa96_loader/fifa96_qfs.h include/fifa96_loader/fifa96_pog.h
git commit -m "feat: add fifa96_envelope header parser with 3-format goldens and GIMX observation" || true
```

---

### Task 2: Codec funnel evidence pass + rename map + concrete header citations + block-walk verdict

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append codec funnel section)
- Modify: `include/fifa96_loader/fifa96_pog.h`, `include/fifa96_loader/fifa96_qfs.h`, `include/fifa96_loader/fifa96_viv.h`, `include/fifa96_loader/fifa96_tgv.h`, `include/fifa96_loader/fifa96_tables.h` (comment lines only)
- Modify: Ghidra program `/fifa96.exe` (evidence-based renames + plate comments only, then `save_program`)

**Interfaces:**
- Consumes: Task 1 `fifa96_envelope_hdr_t` field layout (magic@0, word_a@2, word_b@4, tag@6, tail@10); spec §1 funnel leads (all four lead addresses verified to resolve: `FUN_11bd_5dd2` body `11bd:5dd2..11bd:5faa`, `FUN_1000_0b12` body `1000:0b12..1000:0c0c`, `FUN_11bd_6907` body `11bd:6907..11bd:6955`, `FUN_11bd_5d79` body `11bd:5d79..11bd:5db1`).
- Produces: extended rename map rows `| FUN | address | evidence | new_name | C counterpart |`; the 5 decoder headers cite concrete funnel entries; a written block-walk verdict (adopt with a cited stride, or defer to FU-3). No C behavior change, no new error code.

- [ ] **Step 1: Confirm the funnel at instruction level (Ghidra-MCP, project `fifa96`, program `/fifa96.exe`)**

Run these read-only calls and record the exact output:
1. `decompile_function` on `FUN_11bd_5dd2`, `FUN_1000_0b12`, `FUN_11bd_6907`, `FUN_11bd_5d79`, `FUN_11bd_6102`, `FUN_11bd_26d0`.
2. `get_function_callees` / `get_function_callers` on `FUN_11bd_5dd2` and `FUN_1000_0b12` to confirm the call chain (expected: `5dd2` calls `file_read_far_dos@11bd:6003`, `5d79`, `6907`, `1000:0b12`, `6102`).
3. `search_instructions` for the bit-core markers inside `FUN_1000_0b12` (`SHR AX,CL`, `SHR CX,1`) and record their addresses.
4. `get_xrefs_to` on the global cells the bit-core updates (`0x28b9`, `0x9f1`, `0x9b6`, `0x20`) to source the table-update claim.

Acceptance: the report names, for each examined FUN, either CONFIRMED (one-line evidence: exact instruction or call) or NOT-CONFIRMED (what was missing). Correct the spec's funnel description wherever the disassembly disagrees — the disassembly wins.

- [ ] **Step 2: Rename + plate-comment only what the evidence supports, then save**

For each CONFIRMED funnel function: `rename_function` to an evidence-based name (verb-tier, e.g. `decode_*` only if it demonstrably transforms bytes; otherwise `load_*`/`read_*`), `set_comment(address, comment="C: <counterpart path or 'none — behavioral'>", type=plate)`, then `save_program(program=/fifa96.exe)`. Do NOT rename anything NOT-CONFIRMED. Do NOT invent a FUN address for any decoder.

- [ ] **Step 3: Extend the rename map and the 5 header citations**

Append to `docs/ghidra/loader_rename_map.md` a `## Codec funnel` section with one row per CONFIRMED function:
```markdown
## Codec funnel (verified 2026-09-28, program `/fifa96.exe`)

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_5dd2 | 11bd:5dd2 | <exact call/instruction, e.g. calls file_read_far_dos@11bd:6003> | <new_name> | <path or behavioral> |
```
(One row per CONFIRMED funnel member: `5dd2`, `1000:0b12`, `6907`, `5d79`, `6102`, `26d0` — only rows with filled Evidence survive review; NOT-CONFIRMED members get no row.)

Update the `// Ghidra:` comment in each of the 5 decoder headers to cite the concrete funnel entries (keep the honest behavioral note; replace the bare FU-1 placeholder with `see docs/ghidra/loader_rename_map.md#codec-funnel` plus the specific new names that feed that decoder, or `funnel not yet attributable to this format` where the evidence does not reach).

- [ ] **Step 4: Write the block-walk verdict and verify the suite**

Write the verdict as a `### Block-walk` subsection at the end of the new rename-map section: either ADOPT (state the exact deterministic next-tag rule with the instruction or byte evidence, e.g. fixed stride or in-tail offset field with addresses) or DEFER (state precisely what is missing, which becomes Task 3 FU-3 input). Then run the full suite unchanged:

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — all 9 tests green (no C behavior changed in this task; docs/comments/Ghidra only).

- [ ] **Step 5: Commit**

```bash
git add docs/ghidra/loader_rename_map.md include/fifa96_loader/fifa96_pog.h include/fifa96_loader/fifa96_qfs.h include/fifa96_loader/fifa96_viv.h include/fifa96_loader/fifa96_tgv.h include/fifa96_loader/fifa96_tables.h
git commit -m "docs: map codec funnel with instruction evidence and concrete header citations" || true
```

---

### Task 3: Codec implementation (only if proven) OR FU-3 runtime-capture doc

**Files:**
- Codec path (ONLY if Task 2 verdict is ADOPT with a fully-deterministic algorithm): create `include/fifa96_loader/fifa96_codec.h`, `src/fifa96_loader/fifa96_codec.c`, `tests/test_codec.c`; modify `CMakeLists.txt` (append only).
- FU-3 path (default unless Task 2 proves otherwise): create `docs/ghidra/FU3_codec_runtime_capture.md`; modify `docs/ghidra/FU1_FU2_closeout.md` (append FU-3 section).

**Interfaces:**
- Consumes: Task 2 `### Block-walk` verdict + `## Codec funnel` rows in `docs/ghidra/loader_rename_map.md`.
- Produces (codec path):
  - `fifa96_err_t fifa96_codec_expand(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap, size_t *out_len);`
  - `tests/test_codec.c` asserting byte-exact expansion vectors copied verbatim from the Task 2 verdict (each vector cites its source: instruction addresses + input bytes via `xxd`).
- Produces (FU-3 path): `docs/ghidra/FU3_codec_runtime_capture.md` with algorithm pseudocode + the exact trace experiment; `FU1_FU2_closeout.md` gains the FU-3 section.

- [ ] **Step 1: Apply the Task 2 verdict (no new evidence gathering)**

Read the `### Block-walk` verdict and `## Codec funnel` rows. Take the codec path ONLY if all three hold: (a) exact bit/operand order is cited to instruction addresses, (b) every table/constant source is cited to an address or golden bytes, (c) termination is cited (count field or sentinel with address). Otherwise take the FU-3 path. Record the chosen path and which of (a)–(c) failed (if any) in the commit message.

- [ ] **Step 2a (codec path only): Write the failing test from Task 2 vectors**

```c
// tests/test_codec.c — vectors copied VERBATIM from the Task 2 verdict; no invented bytes
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_codec.h"
int main(void) {
  /* Vector 1: <Task 2 source, e.g. SHPI payload bytes <xxd cmd> -> expected bytes <source>> */
  static const uint8_t in1[] = { /* verbatim Task 2 input bytes */ };
  static const uint8_t exp1[] = { /* verbatim Task 2 expected bytes */ };
  uint8_t out[sizeof(exp1) + 16]; size_t n = 0;
  assert(fifa96_codec_expand(in1, sizeof(in1), out, sizeof(out), &n) == 0);
  assert(n == sizeof(exp1));
  assert(memcmp(out, exp1, n) == 0);
  printf("test_codec OK\n");
  return 0;
}
```

Run: `cmake -S . -B build && cmake --build build 2>&1 | tail -n 5`
Expected: FAIL — `fifa96_loader/fifa96_codec.h: No such file or directory`.

- [ ] **Step 2b (FU-3 path): Write the runtime-capture doc**

Create `docs/ghidra/FU3_codec_runtime_capture.md` with exactly these sections, each filled from the Task 2 evidence (no invented steps):
```markdown
# FU-3 — codec runtime capture

## Algorithm pseudocode (from decompilation)
<per-FUN pseudocode with instruction addresses for every bit/table step>

## Why static proof is insufficient
<which of (a)/(b)/(c) failed, with the exact gap>

## Verification experiment (DOSBox-X INT 21h trace)
1. <exact runner invocation, e.g. ./run-fifa96.sh>
2. <exact INT 21h trace points: read of <golden path> at <FUN address>>
3. <exact capture: input bytes at <buffer>, output bytes at <buffer>>
4. <exact comparison: captured output vs fifa96_codec_expand output>

## Test vectors it would yield
<input xxd command> -> <output xxd command>
```
Append to `docs/ghidra/FU1_FU2_closeout.md`:
```markdown
## FU-3 — codec runtime capture (2026-09-28)
<one paragraph: funnel status, why FU-3, pointer to FU3_codec_runtime_capture.md>
```

- [ ] **Step 3a (codec path only): Write minimal implementation**

```c
// include/fifa96_loader/fifa96_codec.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: <Task 2 funnel citations, e.g. bit-core FUN_1000_0b12 @ 1000:0b12 SHR AX,CL>. Expansion is behaviorally verified by tests/test_codec.c vectors from the Task 2 verdict.
fifa96_err_t fifa96_codec_expand(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap, size_t *out_len);
```

```c
// src/fifa96_loader/fifa96_codec.c
#include "fifa96_loader/fifa96_codec.h"
fifa96_err_t fifa96_codec_expand(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap, size_t *out_len) {
  if (!in || !out || !out_len) return FIFA96_ERR_TRUNCATED;
  /* <Task 2 proven algorithm, every step citing an instruction address> */
  return FIFA96_ERR_TRUNCATED; /* replaced by the proven expansion */
}
```

```cmake
# append to CMakeLists.txt (codec path only)
add_library(fifa96_codec src/fifa96_loader/fifa96_codec.c)
add_executable(test_codec tests/test_codec.c)
target_link_libraries(test_codec PRIVATE fifa96_codec fifa96_file)
add_test(NAME test_codec COMMAND test_codec WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
```

- [ ] **Step 3b (FU-3 path): Verify docs + suite**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — all 9 tests green (FU-3 path changes docs only). Verify: `ls docs/ghidra/FU3_codec_runtime_capture.md` exists and `grep -c "Verification experiment" docs/ghidra/FU3_codec_runtime_capture.md` is nonzero.

- [ ] **Step 4 (codec path): Run test to verify it passes**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build -R test_codec -V`
Expected: PASS — `test_codec OK` with byte-exact vectors.

- [ ] **Step 5: Commit**

Codec path:
```bash
git add include/fifa96_loader/fifa96_codec.h src/fifa96_loader/fifa96_codec.c tests/test_codec.c CMakeLists.txt docs/ghidra/FU1_FU2_closeout.md
git commit -m "feat: add fifa96_codec_expand with Task-2-proven vectors (path: codec, a/b/c hold)" || true
```
FU-3 path:
```bash
git add docs/ghidra/FU3_codec_runtime_capture.md docs/ghidra/FU1_FU2_closeout.md
git commit -m "docs: record FU-3 codec runtime capture (path: FU-3, <which of a/b/c failed>)" || true
```

---

## Self-Review (ran before save)

- Spec coverage: envelope+tests (Task 1 ← spec §2.1/§4/§5), funnel pass+citations+block-walk verdict (Task 2 ← spec §2.2/§1-funnel), codec-or-FU3 (Task 3 ← spec §2.3/§6).
- Placeholder scan: no TBD/TODO; Task 3's conditional is fully specified on both branches (exact FU-3 doc skeleton; codec API + vector harness fed verbatim from Task 2). Task 2's rename rows require filled Evidence or no row.
- Type consistency: `fifa96_err_t` shared via `fifa96_err.h`; parse signature `(const uint8_t*, size_t, out*)` matches slice-1; `tag[4]` non-NUL with `memcmp` matches `fifa96_qfs`; `tail_off/tail_len` are `size_t` matching `fifa96_file_read` out-len; no new error code in Task 1 (header-only), `FIFA96_ERR_BAD_BLOCK` reserved for a Task-2-adopted walk only.
- Golden citations: every magic/size assert uses observed bytes from `xxd -s 0 -l 24` runs above (fw1.qfs 9951 SHPI 0xd400/0xe440 + GIMX@0x12 via `xxd -s 0x12 -l 4`; pcindex.pog 25857 PCNX 0xb600/0xe140; gameart0.pvi 154387 BIGF 0x1704/0xe3a2).

(End of file)
