# FIFA96 Script Semantics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Attribute the else-branch input files, pin down script keyword/branch semantics with instruction evidence, and port the provable word-table mechanics to a golden-tested `fifa96_script` C library.

**Architecture:** Ghidra evidence pass first (filename attribution at open sites + keyword branch bodies), then a pure bytes-in `fifa96_script` lib implementing ONLY Task-1-CONFIRMED mechanics (table append/overflow, `0xffff` terminator, `;`-comment skip gated on confirmed skip behavior, golden vectors gated on attribution), then citations + regression.

**Tech Stack:** C11, gcc 13.3, CMake 3.28, CTest, `xxd` (golden citations), Ghidra-MCP (`decompile_function`, `get_xrefs_*`, `read_memory`/`inspect_memory`, `rename_function`, `set_comment`, `save_program`).

**Spec:** `docs/superpowers/specs/2026-09-28-fifa96-script-semantics-design.md`

## Global Constraints

- C11 only, flags `-Wall -Wextra -Werror`.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`; goldens are copies under `tests/golden/`.
- No byte offset, byte value, or algorithm step enters code without a golden/`xxd`/instruction citation.
- All byte assertions in tests must be derivable by `xxd -s OFF -l N` on the committed golden (cite the command in the test comment); synthetic mechanism vectors cite the motivating instruction address instead.
- No keyword meaning enters C without an instruction or golden pair proving it; unproven branches stay out of C and in the report.
- If no golden input file is attributable, the slice ships the semantics report + map rows with NO `fifa96_script` lib.
- Ghidra work targets project `fifa96`, program `/fifa96.exe`, with `save_program` after each rename batch. Do NOT invent FUN addresses. No `decode_*` names.
- Slice-1/2 APIs frozen (comments only).

---

## Scope Check

Spec slice 4 covers one subsystem (script text/word-table) in three gates: attribution + keyword evidence (Task 1, docs-only), C mechanics port (Task 2, code + tests), citations + regression (Task 3, docs). Each task ends with an independently testable deliverable. No split needed.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## Script semantics` section (Tasks 1, 3).
- Create: `include/fifa96_loader/fifa96_script.h`, `src/fifa96_loader/fifa96_script.c` (Task 2, ONLY if the Task-1 gate below passes).
- Create: `tests/test_script.c` (Task 2, same gate).
- Modify: `CMakeLists.txt` — append `fifa96_script` lib + `test_script` (Task 2, same gate).
- Modify: `include/fifa96_loader/fifa96_{pog,qfs,viv,tgv,tables}.h` — comment lines only, ONLY on observed attribution (Task 3).

---

### Task 1: Filename attribution + keyword/branch evidence pass

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## Script semantics` evidence section)
- Modify: Ghidra program `/fifa96.exe` (evidence-based renames + plate comments, CONFIRMED only, then `save_program`)

**Interfaces:**
- Consumes: verified bodies — `parse_script_text` (`11bd:5bdb..11bd:5c8a`), `build_word_table` (`11bd:5c8b..11bd:5d78`), `dispatch_object_load` (`11bd:5992..11bd:5ad5`); known open call sites: `CALL file_open_dos` at `11bd:5c5a`, `CALL file_close_dos` at `11bd:5c47`, quote dispatch `SUB AX,0x23` at `11bd:5ca2`, C/E/M dispatch at `11bd:5cb7`/`5cbd`/`5cc0`, terminator at `11bd:5d18`, publish at `11bd:5d1e`/`5d26`.
- Produces: attributed filename strings with addresses (or NOT-ATTRIBUTABLE with the indirection reason); per-keyword verdicts (E/R meaning, C/E/M branch behavior, `;` compare-vs-skip, quote rule, include rule, terminator, limit/grow, publication); map rows with filled Evidence. The Task-2 gate: `fifa96_script` lib is built ONLY if at least the table mechanics (append/terminator) are CONFIRMED here.

- [ ] **Step 1: Attribute the input filenames at open sites**

Run (project `fifa96`, program `/fifa96.exe`):
1. `decompile_function` on `FUN_11bd_5bdb`; identify the filename argument to the `file_open_dos` call at `11bd:5c5a` (register or stack slot holding the name pointer).
2. Resolve the bytes: `read_memory`/`inspect_memory_content` at the pointer target (or `get_xrefs_to` the string address); record the exact bytes + address.
3. If the pointer is passed in (not a local literal): walk ONE caller up (`get_function_callers` on `5bdb`, then decompile the caller) and repeat. If still indirect after one level: verdict NOT-ATTRIBUTABLE with the indirection reason.
4. Repeat for any other `file_open_dos` call site on the else-branch path found in `dispatch_object_load`'s decompile.

Acceptance: each filename recorded as `"<bytes>" @ <address> via <call site>` with the read command that produced it, or NOT-ATTRIBUTABLE with the exact gap. No filename guessing: unresolvable pointers are reported, not named.

- [ ] **Step 2: Pin down keyword/branch semantics**

`decompile_function` on `FUN_11bd_5bdb` (full) and `FUN_11bd_5c8b` (full); for each mechanism record CONFIRMED-with-behavior or OPEN:
1. `;` rule: `SUB AX,0x3c` at `11bd:5be9` — compare only, or compare + advance-past-newline (cite the advance instructions or report OPEN).
2. Quote rule: `SUB AX,0x23` at `11bd:5ca2` — what each quote byte (`0x22`/`0x27`) DOES (cite).
3. C/E/M branches at `11bd:5cb7`/`5cbd`/`5cc0` — per-branch behavior (cite); E/R meaning — whatever `E`/`R` turn out to be (chars, flags, branches — cite, do not assume).
4. Include rule: close at `11bd:5c47` + reopen at `11bd:5c5a` — filename source for the reopen (same method as Step 1).
5. Return rule of `5bdb` (0/1 conditions at `11bd:5c16` compare).
6. Table mechanics: append store at `11bd:5d44`, limit check at `11bd:5d4d`, grow call at `11bd:5d55`, terminator store at `11bd:5d18`, publish at `11bd:5d1e`/`5d26` — all already cited; re-confirm against the fresh decompile (one line each).

- [ ] **Step 3: Rename CONFIRMED newcomers only, then save**

`rename_function` + plate `set_comment` only for members whose role this pass newly confirms; then `save_program(program=/fifa96.exe)`. No renames for OPEN items. No `decode_*`.

- [ ] **Step 4: Append the map evidence section**

Append to `docs/ghidra/loader_rename_map.md`:
```markdown
## Script semantics (verified 2026-09-28, program `/fifa96.exe`)

Attribution: <"<bytes>" @ <address> via <call site> | NOT-ATTRIBUTABLE: <reason>>

| Item | Verdict | Evidence |
|------|---------|----------|
| `;` rule | CONFIRMED-skip \| CONFIRMED-compare-only \| OPEN | <instructions or gap> |
| quote rule | <verdict> | <instructions or gap> |
| C/E/M branches | <per-branch verdicts> | <instructions or gap> |
| E/R meaning | <verdict> | <instructions or gap> |
| include reopen | <verdict> | <instructions or gap> |
| 5bdb return rule | <verdict> | <instructions or gap> |
| table append/limit/grow/terminator/publish | CONFIRMED | <5d44/5d4d/5d55/5d18/5d1e/5d26> |
```
(Every row filled; OPEN rows state the exact gap. The Task-2 gate line, verbatim: `Gate for fifa96_script lib: <PASS with confirmed list | FAIL with reason>`.)

- [ ] **Step 5: Verify suite regression and commit**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — 9/9 green (docs/Ghidra only).

```bash
git add docs/ghidra/loader_rename_map.md
git commit -m "docs: attribute script inputs and pin keyword semantics with evidence" || true
```

---

### Task 2: fifa96_script mechanics port + tests (gate: Task-1 PASS)

**Files:**
- Create: `include/fifa96_loader/fifa96_script.h`
- Create: `src/fifa96_loader/fifa96_script.c`
- Create: `tests/test_script.c`
- Modify: `CMakeLists.txt` (append only)

**Interfaces:**
- Consumes: Task-1 `## Script semantics` verdicts + gate line; `fifa96_err_t` from `fifa96_err.h`.
- Produces:
  - `typedef struct { uint16_t *words; size_t len; size_t cap; } fifa96_script_table_t;`
  - `fifa96_err_t fifa96_script_table_append(fifa96_script_table_t *t, uint16_t v);` — `FIFA96_OK` on append; `FIFA96_ERR_TRUNCATED` on NULL table/words or full cap. (Fixed-cap by design: the DOS grow idiom is allocator business; callers grow. Documented in header.)
  - `fifa96_err_t fifa96_script_table_terminated(const uint16_t *w, size_t n, size_t *term_at);` — `FIFA96_OK` + index of first `0xffff` (instr `MOV word ptr [SI],0xffff` at `11bd:5d18`); `FIFA96_ERR_TRUNCATED` on NULL, or no terminator in `n`.
  - `size_t fifa96_script_comment_len(const uint8_t *b, size_t n);` — bytes from a leading `;` through the newline inclusive (or `n` if none); 0 unless `n > 0 && b[0] == 0x3b` (instr `SUB AX,0x3c` at `11bd:5be9`). **Implement + test ONLY if Task-1 `;` row is CONFIRMED-skip; otherwise omit entirely and record OPEN.**
- Gate: if Task-1 gate line is FAIL, create NO lib/test/CMake entries; instead append the FAIL reason to the map section and commit docs-only (`git add docs/ghidra/loader_rename_map.md && git commit -m "docs: record script-lib gate FAIL (no attributable inputs)"`). All steps below assume PASS.

- [ ] **Step 1: Write the failing test**

```c
// tests/test_script.c — synthetic mechanism vectors cite motivating instructions; golden vectors (if any) cite xxd + Task-1 attribution
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_script.h"
int main(void) {
  uint16_t w[4]; fifa96_script_table_t t = { w, 0, 4 };
  size_t at = 0;
  /* table mechanics: terminator 0xffff per MOV word ptr [SI],0xffff at 11bd:5d18 */
  assert(fifa96_script_table_append(&t, 0x0043) == 0);   /* 'C' dispatch byte 0x43 at 11bd:5cb7 */
  assert(fifa96_script_table_append(&t, 0x0045) == 0);   /* 'E' dispatch byte 0x45 at 11bd:5cbd */
  assert(fifa96_script_table_append(&t, 0x004d) == 0);   /* 'M' dispatch byte 0x4d at 11bd:5cc0 */
  assert(t.len == 3);
  assert(fifa96_script_table_append(&t, 0xffff) == 0);
  assert(fifa96_script_table_terminated(w, t.len, &at) == 0 && at == 3);
  assert(fifa96_script_table_append(&t, 0x0001) == FIFA96_ERR_TRUNCATED); /* cap 4 full: limit idiom CMP AX,SI at 11bd:5d4d */
  /* negatives */
  assert(fifa96_script_table_append(0, 1) == FIFA96_ERR_TRUNCATED);
  { fifa96_script_table_t n = { 0, 0, 0 }; assert(fifa96_script_table_append(&n, 1) == FIFA96_ERR_TRUNCATED); }
  assert(fifa96_script_table_terminated(0, 1, &at) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_script_table_terminated(w, 3, 0) == FIFA96_ERR_TRUNCATED);
  { static const uint16_t u[] = { 0x15e8, 0x0001 }; assert(fifa96_script_table_terminated(u, 2, &at) == FIFA96_ERR_TRUNCATED); } /* no 0xffff in range */
  /* comment skip (ONLY if Task-1 ';' row is CONFIRMED-skip; delete this block otherwise) */
  { static const uint8_t c[] = { 0x3b, 0x41, 0x0a, 0x42 }; assert(fifa96_script_comment_len(c, 4) == 3); } /* ';A\nB': SUB AX,0x3c at 11bd:5be9 */
  { static const uint8_t c[] = { 0x41, 0x0a }; assert(fifa96_script_comment_len(c, 2) == 0); }
  assert(fifa96_script_comment_len(0, 4) == 0);
  /* golden vectors (ONLY files Task 1 attributed; bytes verbatim, each line cites xxd + attribution) */
  printf("test_script OK\n");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake -S . -B build && cmake --build build 2>&1 | tail -n 5`
Expected: FAIL — `fifa96_loader/fifa96_script.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

```c
// include/fifa96_loader/fifa96_script.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#define FIFA96_SCRIPT_TERM 0xffffu
typedef struct { uint16_t *words; size_t len; size_t cap; } fifa96_script_table_t;
// Ghidra: else-branch parse_script_text@11bd:5bdb + build_word_table@11bd:5c8b (verified; see docs/ghidra/loader_rename_map.md#script-semantics). Mechanics mirrored here: append (store at 11bd:5d44), full-cap TRUNCATED (limit idiom CMP AX,SI at 11bd:5d4d; DOS grow via mem_grow_relocate at 11bd:5d55 is allocator business — fixed-cap by design, callers grow), 0xffff terminator (store at 11bd:5d18), ;-comment skip (SUB AX,0x3c at 11bd:5be9; present only if Task-1 CONFIRMED-skip). Keyword branch semantics (C/E/M/E/R) stay out of C until proven — see map.
fifa96_err_t fifa96_script_table_append(fifa96_script_table_t *t, uint16_t v);
fifa96_err_t fifa96_script_table_terminated(const uint16_t *w, size_t n, size_t *term_at);
size_t fifa96_script_comment_len(const uint8_t *b, size_t n);
```

```c
// src/fifa96_loader/fifa96_script.c
#include "fifa96_loader/fifa96_script.h"
fifa96_err_t fifa96_script_table_append(fifa96_script_table_t *t, uint16_t v) {
  if (!t || !t->words || t->len >= t->cap) return FIFA96_ERR_TRUNCATED;
  t->words[t->len++] = v;
  return FIFA96_OK;
}
fifa96_err_t fifa96_script_table_terminated(const uint16_t *w, size_t n, size_t *term_at) {
  size_t i = 0;
  if (!w || !term_at) return FIFA96_ERR_TRUNCATED;
  for (i = 0; i < n; i++) {
    if (w[i] == FIFA96_SCRIPT_TERM) { *term_at = i; return FIFA96_OK; }
  }
  return FIFA96_ERR_TRUNCATED;
}
size_t fifa96_script_comment_len(const uint8_t *b, size_t n) {
  size_t i = 0;
  if (!b || n == 0 || b[0] != 0x3b) return 0;
  for (i = 1; i < n; i++) {
    if (b[i] == 0x0a) return i + 1;
  }
  return n;
}
```

```cmake
# append to CMakeLists.txt
add_library(fifa96_script src/fifa96_loader/fifa96_script.c)
add_executable(test_script tests/test_script.c)
target_link_libraries(test_script PRIVATE fifa96_script fifa96_file)
add_test(NAME test_script COMMAND test_script WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
```

(Omit `fifa96_script_comment_len` decl+defn+tests if Task-1 `;` row is not CONFIRMED-skip; reviewer checks the gate was applied.)

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build -V`
Expected: PASS — `test_script OK`, full suite green (10/10 with the lib path; 9/9 on the gate-FAIL docs path).

- [ ] **Step 5: Commit**

```bash
git add include/fifa96_loader/fifa96_script.h src/fifa96_loader/fifa96_script.c tests/test_script.c CMakeLists.txt
git commit -m "feat: add fifa96_script word-table mechanics with instruction-cited vectors" || true
```

---

### Task 3: Header citations + regression closeout

**Files:**
- Modify: `include/fifa96_loader/fifa96_{pog,qfs,viv,tgv,tables}.h` — comment lines only, ONLY on Task-1-observed attribution (default: untouched).
- Modify: `docs/ghidra/loader_rename_map.md` — one-line closeout pointer if headers were touched (else untouched).

**Interfaces:**
- Consumes: Task-1 attribution verdicts; Task-2 lib/tests (or gate-FAIL record).
- Produces: concrete header citations where attribution reaches a format; full-suite green confirmation. No new code, no new error codes.

- [ ] **Step 1: Update header citations only on observed attribution**

For each decoder header: if Task 1 attributed an input file to that format's path (filename bytes at an open site on its load path), extend its `// Ghidra:` line with `input <bytes> @ <address> (see loader_rename_map.md#script-semantics)`. Otherwise touch nothing. Never guess a filename.

- [ ] **Step 2: Verify suite + content and commit**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — full suite green (10/10 lib path, 9/9 gate-FAIL path). Verify: `grep -c "Script semantics" docs/ghidra/loader_rename_map.md` is nonzero.

```bash
git add docs/ghidra/loader_rename_map.md include/fifa96_loader/fifa96_pog.h include/fifa96_loader/fifa96_qfs.h include/fifa96_loader/fifa96_viv.h include/fifa96_loader/fifa96_tgv.h include/fifa96_loader/fifa96_tables.h
git commit -m "docs: cite script input attribution in decoder headers" || true
```
(Headers untouched by Step 1 contribute nothing to the diff.)

---

## Self-Review (ran before save)

- Spec coverage: attribution + keyword evidence (Task 1 ← spec §2.1–2.2), C mechanics port gated (Task 2 ← spec §2.3/§4/§5), citations + regression (Task 3 ← spec §2.4). Trace/FU-2/boot exclusions hold (§7).
- Placeholder scan: no TBD/TODO; Task-2 conditional content is fully specified on both sides (comment-gate omit rule, golden-bytes-verbatim rule, gate-FAIL docs commit); Task-3 header edits conditional with explicit default.
- Type consistency: `fifa96_err_t` shared; `(bytes,size,out)` idiom matches slice-1; `tag`-style non-NUL not used (u16 words + raw bytes); `FIFA96_SCRIPT_TERM 0xffffu` matches terminator store width; `size_t` lengths match `fifa96_file_read` out-len; no new error code.
- Instruction citations in Task-2 vectors/decls match spec §1 addresses (`5be3/5be9/5ca2/5cb7/5cbd/5cc0/5d18/5d44/5d4d/5d55/5c47/5c5a/5c16`).

(End of file)
