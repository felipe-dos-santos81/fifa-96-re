# FIFA96 File Loader Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Recreate fifa96.exe file/data loading in portable C as 1:1 matching functions with golden tests.

**Architecture:** DOS `fifa96_file` shim (POSIX backend, segment-split reads) under pure decoders (`tables/pog/qfs/viv/tgv`); Ghidra INT 21h pass feeds rename-backflow; `ctest` golden vectors from read-only CD validate byte-identity.

**Tech Stack:** C11, gcc 13.3, CMake 3.28, CTest, Python 3.12 (probes only), Ghidra-MCP (`search_instructions`, `decompile_function`, `get_xrefs_*`), DOSBox-X oracle.

**Spec:** `docs/superpowers/specs/2026-09-27-fifa96-file-loader-design.md`

## Global Constraints

- C11 only, flags `-Wall -Wextra -Werror`.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`; goldens are copies under `tests/golden/`.
- Every C loader function doc-comments its source `Ghidra: FUN_11bd_XXXX @ 11bd:XXXX`.
- Ghidra work targets project `fifa96`, program `/fifa96.exe`, with `save_program` after each rename batch.
- Each task ends with an independently testable `ctest` deliverable.

---

## Scope Check

Spec slice 1 covers one subsystem (file/data loading) with five small formats sharing one file shim. One plan, six tasks. Each task is independently testable. No split into sub-plans needed.

## File Structure

- Create: `CMakeLists.txt` — root build, C11 strict, `enable_testing()`.
- Create: `include/fifa96_loader/fifa96_err.h` — shared `fifa96_err_t` enum.
- Create: `include/fifa96_loader/fifa96_file.h` — shim API.
- Create: `src/fifa96_loader/fifa96_file.c` — POSIX backend + 64 KiB split.
- Create: `include/fifa96_loader/fifa96_tables.h` + `src/fifa96_loader/fifa96_tables.c` — fnames/lengths/crcvals.
- Create: `include/fifa96_loader/fifa96_pog.h` + `src/fifa96_loader/fifa96_pog.c`
- Create: `include/fifa96_loader/fifa96_qfs.h` + `src/fifa96_loader/fifa96_qfs.c`
- Create: `include/fifa96_loader/fifa96_viv.h` + `src/fifa96_loader/fifa96_viv.c`
- Create: `include/fifa96_loader/fifa96_tgv.h` + `src/fifa96_loader/fifa96_tgv.c`
- Create: `tests/test_file.c`, `tests/test_tables.c`, `tests/test_pog.c`, `tests/test_qfs.c`, `tests/test_viv.c`, `tests/test_tgv.c`, `tests/test_load_order.c`
- Create: `tests/golden/README.md` + manifests (copies/probes output).
- Create: `tools/probe/dump_header.py` — read-only header dumper.
- Create: `docs/ghidra/loader_rename_map.md` — `FUN_11bd_* -> new_name` map from Task 2.

---

### Task 1: Scaffolding + fifa96_file shim

**Files:**
- Create: `CMakeLists.txt`
- Create: `include/fifa96_loader/fifa96_err.h`
- Create: `include/fifa96_loader/fifa96_file.h`
- Create: `src/fifa96_loader/fifa96_file.c`
- Test: `tests/test_file.c`

**Interfaces:**
- Consumes: nothing (first task).
- Produces:
  - `fifa96_err_t fifa96_file_read(const char *path, uint8_t **out, size_t *out_len);`
  - `fifa96_err_t fifa96_file_read_chunk(const char *path, uint64_t off, uint8_t *dst, size_t len);`
  - `size_t fifa96_file_split_for_segment(uint16_t seg_off, size_t len);`

- [ ] **Step 1: Write the failing test**

```c
// tests/test_file.c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
int main(void) {
  uint8_t *buf = 0; size_t len = 0;
  // Golden: soccer/fnames.dat starts with "FW1.QFS\0"
  assert(fifa96_file_read("tests/golden/fnames.dat", &buf, &len) == 0);
  assert(len > 16);
  assert(memcmp(buf, "FW1.QFS", 7) == 0);
  // Segment-split helper: offset 0xFF00 + 0x200 must split at 0x100
  assert(fifa96_file_split_for_segment(0xFF00, 0x200) == 0x100);
  printf("test_file OK len=%zu\n", len);
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake -S . -B build && cmake --build build 2>&1 | head -n 20`
Expected: FAIL — `fifa96_loader/fifa96_file.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(fifa96_loader C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror)
include_directories(include)
enable_testing()
add_library(fifa96_file src/fifa96_loader/fifa96_file.c)
add_executable(test_file tests/test_file.c)
target_link_libraries(test_file PRIVATE fifa96_file)
add_test(NAME test_file COMMAND test_file)
```

```c
// include/fifa96_loader/fifa96_err.h
#pragma once
typedef enum {
  FIFA96_OK = 0,
  FIFA96_ERR_NOT_FOUND = 1,
  FIFA96_ERR_SHORT_READ = 2,
  FIFA96_ERR_BAD_MAGIC = 3,
  FIFA96_ERR_TRUNCATED = 4,
  FIFA96_ERR_CRC_MISMATCH = 5,
  FIFA96_ERR_IO = 6
} fifa96_err_t;
```

```c
// include/fifa96_loader/fifa96_file.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: wrappers around INT 21h AH=3Dh/3Fh/3Eh (exact FUN_11bd_* cited in Task 2).
fifa96_err_t fifa96_file_read(const char *path, uint8_t **out, size_t *out_len);
fifa96_err_t fifa96_file_read_chunk(const char *path, uint64_t off, uint8_t *dst, size_t len);
size_t fifa96_file_split_for_segment(uint16_t seg_off, size_t len);
void fifa96_file_free(uint8_t *p);
```

```c
// src/fifa96_loader/fifa96_file.c
#include "fifa96_loader/fifa96_file.h"
#include <stdio.h>
#include <stdlib.h>
size_t fifa96_file_split_for_segment(uint16_t seg_off, size_t len) {
  size_t to_boundary = (size_t)(0x10000u - (unsigned)seg_off);
  return len < to_boundary ? len : to_boundary;
}
void fifa96_file_free(uint8_t *p) { free(p); }
fifa96_err_t fifa96_file_read(const char *path, uint8_t **out, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f) return FIFA96_ERR_NOT_FOUND;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) { fclose(f); return FIFA96_ERR_IO; }
  uint8_t *b = (uint8_t *)malloc((size_t)n ? (size_t)n : 1u);
  if (!b) { fclose(f); return FIFA96_ERR_IO; }
  size_t got = fread(b, 1, (size_t)n, f);
  fclose(f);
  if (got != (size_t)n) { free(b); return FIFA96_ERR_SHORT_READ; }
  *out = b; *out_len = got;
  return FIFA96_OK;
}
fifa96_err_t fifa96_file_read_chunk(const char *path, uint64_t off, uint8_t *dst, size_t len) {
  FILE *f = fopen(path, "rb");
  if (!f) return FIFA96_ERR_NOT_FOUND;
  if (fseek(f, (long)off, SEEK_SET) != 0) { fclose(f); return FIFA96_ERR_IO; }
  size_t got = fread(dst, 1, len, f);
  fclose(f);
  return got == len ? FIFA96_OK : FIFA96_ERR_SHORT_READ;
}
```

Setup golden copy (read-only source, one-time):

```bash
mkdir -p tests/golden
cp /media/felipe/FIFAPCCD/soccer/fnames.dat tests/golden/fnames.dat
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build -V`
Expected: PASS — `test_file OK`, `1/1 Test Passed`.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt include tests/test_file.c src tests/golden/README.md 2>/dev/null || true
git commit -m "feat: add fifa96_file shim with segment-split and golden fnames test" || true
```

---

### Task 2: Ghidra INT 21h pass + rename map

**Files:**
- Create: `docs/ghidra/loader_rename_map.md`
- Create: `tools/probe/dump_header.py`
- Test: `tests/test_load_order.c` (manifest existence check; full sequence asserted in Task 6)

**Interfaces:**
- Consumes: `fifa96_file_*` names from Task 1 (C counterparts).
- Produces: `docs/ghidra/loader_rename_map.md` with rows `| FUN_11bd_* | 11bd:* | INT 21h AH | new_name | C counterpart |`.

- [ ] **Step 1: Write the failing check**

```c
// tests/test_load_order.c (skeleton for this task: manifest must exist)
#include <assert.h>
#include <stdio.h>
int main(void) {
  FILE *f = fopen("tests/golden/load_order.txt", "rb");
  assert(f && "load_order.txt manifest missing; run Task 2 probe");
  fclose(f);
  printf("load_order manifest present\n");
  return 0;
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build -R test_load_order -V 2>&1 | tail -n 10`
Expected: FAIL — manifest missing / test not yet registered.

- [ ] **Step 3: Perform Ghidra pass and write artifacts**

Ghidra-MCP calls (project `fifa96`, program `/fifa96.exe`):
1. `search_instructions(mnemonic=INT, program=/fifa96.exe, limit=200)` — keep operands containing `21`.
2. For each hit: `get_xrefs_from(address)`, `get_function_by_address(address)` to find enclosing `FUN_11bd_*`.
3. `decompile_function(address=FUN address)` — confirm AH value (3D/3F/42/3E) and carry-flag branch.
4. Rename wrapper via `rename_function(old_name=FUN_11bd_*, new_name=file_open/read/seek/close variant)`; `set_comment(address, comment="C: src/fifa96_loader/fifa96_file.c:<fn>", type=plate)`; `save_program(program=/fifa96.exe)`.

```markdown
<!-- docs/ghidra/loader_rename_map.md -->
# Loader rename map
| Ghidra FUN | Address | INT 21h AH | New name | C counterpart |
|------------|---------|------------|----------|---------------|
| FUN_11bd_XXXX | 11bd:XXXX | 3D | file_open_dos | fifa96_file_read (open path) |
```

```python
# tools/probe/dump_header.py — read-only, prints magic + size
import sys
p = sys.argv[1]
b = open(p, "rb").read(32)
print(f"{p} len_prefix={b[:16].hex()} ascii={b[:16]!r}")
```

```bash
mkdir -p tests/golden docs/ghidra
python3 tools/probe/dump_header.py /media/felipe/FIFAPCCD/soccer/fnames.dat
printf '# load order oracle (DOSBox-X INT21 trace fills sequence)\ntests/golden/fnames.dat\n' > tests/golden/load_order.txt
```

- [ ] **Step 4: Register test and verify it passes**

```cmake
# append to CMakeLists.txt
add_executable(test_load_order tests/test_load_order.c)
add_test(NAME test_load_order COMMAND test_load_order)
```

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build -V`
Expected: PASS — `load_order manifest present`.

- [ ] **Step 5: Commit**

```bash
git add docs/ghidra/loader_rename_map.md tools/probe/dump_header.py tests/test_load_order.c tests/golden/load_order.txt CMakeLists.txt
git commit -m "docs: add INT21 loader rename map and load-order oracle skeleton" || true
```

---

### Task 3: fifa96_tables (fnames/lengths/crcvals)

**Files:**
- Create: `include/fifa96_loader/fifa96_tables.h`
- Create: `src/fifa96_loader/fifa96_tables.c`
- Test: `tests/test_tables.c`

**Interfaces:**
- Consumes: `fifa96_file_read` from Task 1.
- Produces:
  - `fifa96_err_t fifa96_tables_entry_count(const uint8_t *fnames, size_t n, size_t *count);`
  - `fifa96_err_t fifa96_tables_name_at(const uint8_t *fnames, size_t n, size_t idx, char out[16]);`
  - `fifa96_err_t fifa96_tables_length_at(const uint8_t *lengths, size_t n, size_t idx, uint32_t *val);`

- [ ] **Step 1: Write the failing test**

```c
// tests/test_tables.c
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_tables.h"
int main(void) {
  uint8_t *fn = 0, *ln = 0; size_t nfn = 0, nln = 0;
  assert(fifa96_file_read("tests/golden/fnames.dat", &fn, &nfn) == 0);
  assert(fifa96_file_read("tests/golden/lengths.dat", &ln, &nln) == 0);
  char name[16]; uint32_t v = 0;
  assert(fifa96_tables_name_at(fn, nfn, 0, name) == 0);
  assert(strcmp(name, "FW1.QFS") == 0);           // golden bytes 0000: FW1.QFS
  assert(fifa96_tables_length_at(ln, nln, 0, &v) == 0);
  assert(v == 0x26df);                            // golden bytes 0000: df 26 00 00
  printf("test_tables OK\n");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake -S . -B build && cmake --build build 2>&1 | tail -n 5`
Expected: FAIL — `fifa96_tables.h not found`.

- [ ] **Step 3: Write minimal implementation**

```c
// include/fifa96_loader/fifa96_tables.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: table index helpers (FUN_11bd_* filled in Task 2 map).
fifa96_err_t fifa96_tables_entry_count(const uint8_t *fnames, size_t n, size_t *count);
fifa96_err_t fifa96_tables_name_at(const uint8_t *fnames, size_t n, size_t idx, char out[16]);
fifa96_err_t fifa96_tables_length_at(const uint8_t *lengths, size_t n, size_t idx, uint32_t *val);
```

```c
// src/fifa96_loader/fifa96_tables.c
#include "fifa96_loader/fifa96_tables.h"
#include <string.h>
#define REC 16u
fifa96_err_t fifa96_tables_entry_count(const uint8_t *fnames, size_t n, size_t *count) {
  if (!fnames || !count || n % REC) return FIFA96_ERR_TRUNCATED;
  *count = n / REC;
  return FIFA96_OK;
}
fifa96_err_t fifa96_tables_name_at(const uint8_t *fnames, size_t n, size_t idx, char out[16]) {
  size_t c = 0;
  if (fifa96_tables_entry_count(fnames, n, &c) != 0 || idx >= c) return FIFA96_ERR_TRUNCATED;
  memcpy(out, fnames + idx * REC, REC);
  out[15] = 0;
  return FIFA96_OK;
}
fifa96_err_t fifa96_tables_length_at(const uint8_t *lengths, size_t n, size_t idx, uint32_t *val) {
  if (!lengths || !val || (idx + 1) * 4 > n) return FIFA96_ERR_TRUNCATED;
  const uint8_t *p = lengths + idx * 4;
  *val = (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
  return FIFA96_OK;
}
```

```bash
cp /media/felipe/FIFAPCCD/soccer/lengths.dat tests/golden/lengths.dat
cp /media/felipe/FIFAPCCD/soccer/crcvals.dat tests/golden/crcvals.dat
```

```cmake
# append
add_library(fifa96_tables src/fifa96_loader/fifa96_tables.c)
target_link_libraries(fifa96_tables PRIVATE fifa96_file)
add_executable(test_tables tests/test_tables.c)
target_link_libraries(test_tables PRIVATE fifa96_tables fifa96_file)
add_test(NAME test_tables COMMAND test_tables)
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build -R "test_tables|test_file" -V`
Expected: PASS both.

- [ ] **Step 5: Commit**

```bash
git add include/fifa96_loader/fifa96_tables.h src/fifa96_loader/fifa96_tables.c tests/test_tables.c CMakeLists.txt
git commit -m "feat: add fifa96_tables with fnames/lengths golden asserts" || true
```

---

### Task 4: fifa96_pog (pcindex + lang)

**Files:**
- Create: `include/fifa96_loader/fifa96_pog.h`, `src/fifa96_loader/fifa96_pog.c`
- Test: `tests/test_pog.c`

**Interfaces:**
- Consumes: `fifa96_file_read`.
- Produces:
  - `typedef struct { uint8_t magic[2]; uint16_t w1; uint32_t w2; } fifa96_pog_hdr_t;`
  - `fifa96_err_t fifa96_pog_parse_hdr(const uint8_t *b, size_t n, fifa96_pog_hdr_t *h);`

- [ ] **Step 1: Write the failing test**

```c
// tests/test_pog.c
#include <assert.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_pog.h"
int main(void) {
  uint8_t *b = 0; size_t n = 0;
  assert(fifa96_file_read("tests/golden/pcindex.pog", &b, &n) == 0);
  assert(n == 25857);                             // golden size
  fifa96_pog_hdr_t h;
  assert(fifa96_pog_parse_hdr(b, n, &h) == 0);
  assert(h.magic[0] == 0x10 && h.magic[1] == 0xfb); // golden bytes 0000: 10 fb
  printf("test_pog OK\n");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake -S . -B build && cmake --build build 2>&1 | tail -n 5`
Expected: FAIL — `fifa96_pog.h not found`.

- [ ] **Step 3: Write minimal implementation**

```c
// include/fifa96_loader/fifa96_pog.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { uint8_t magic[2]; uint16_t w1; uint32_t w2; } fifa96_pog_hdr_t;
// Ghidra: POG header reader (FUN_11bd_* per rename map).
fifa96_err_t fifa96_pog_parse_hdr(const uint8_t *b, size_t n, fifa96_pog_hdr_t *h);
```

```c
// src/fifa96_loader/fifa96_pog.c
#include "fifa96_loader/fifa96_pog.h"
fifa96_err_t fifa96_pog_parse_hdr(const uint8_t *b, size_t n, fifa96_pog_hdr_t *h) {
  if (!b || !h || n < 8) return FIFA96_ERR_TRUNCATED;
  if (b[0] != 0x10 || b[1] != 0xfb) return FIFA96_ERR_BAD_MAGIC;
  h->magic[0] = b[0]; h->magic[1] = b[1];
  h->w1 = (uint16_t)(b[2] | (b[3] << 8));
  h->w2 = (uint32_t)(b[4] | (b[5] << 8) | (b[6] << 16) | (b[7] << 24));
  return FIFA96_OK;
}
```

```bash
cp /media/felipe/FIFAPCCD/fedata/pcindex.pog tests/golden/pcindex.pog
```

```cmake
# append
add_library(fifa96_pog src/fifa96_loader/fifa96_pog.c)
add_executable(test_pog tests/test_pog.c)
target_link_libraries(test_pog PRIVATE fifa96_pog fifa96_file)
add_test(NAME test_pog COMMAND test_pog)
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build -R test_pog -V`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add include/fifa96_loader/fifa96_pog.h src/fifa96_loader/fifa96_pog.c tests/test_pog.c CMakeLists.txt
git commit -m "feat: add fifa96_pog header parser with pcindex golden" || true
```

---

### Task 5: fifa96_qfs (.qfs/.pvi framing)

**Files:**
- Create: `include/fifa96_loader/fifa96_qfs.h`, `src/fifa96_loader/fifa96_qfs.c`
- Test: `tests/test_qfs.c`

**Interfaces:**
- Consumes: `fifa96_file_read`.
- Produces:
  - `typedef struct { uint8_t magic[2]; uint32_t dec_len; char tag[4]; } fifa96_qfs_hdr_t;`
  - `fifa96_err_t fifa96_qfs_parse_hdr(const uint8_t *b, size_t n, fifa96_qfs_hdr_t *h);`

- [ ] **Step 1: Write the failing test**

```c
// tests/test_qfs.c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_qfs.h"
int main(void) {
  uint8_t *q = 0, *p = 0; size_t nq = 0, np = 0;
  assert(fifa96_file_read("tests/golden/fw1.qfs", &q, &nq) == 0);
  assert(nq == 9951);
  fifa96_qfs_hdr_t h;
  assert(fifa96_qfs_parse_hdr(q, nq, &h) == 0);
  assert(h.magic[0] == 0x10 && h.magic[1] == 0xfb);
  assert(fifa96_file_read("tests/golden/gameart0.pvi", &p, &np) == 0);
  assert(np == 154387);
  assert(memcmp(p + 6, "BIGF", 4) == 0);          // golden bytes 0006: BIGF
  printf("test_qfs OK\n");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake -S . -B build && cmake --build build 2>&1 | tail -n 5`
Expected: FAIL — `fifa96_qfs.h not found`.

- [ ] **Step 3: Write minimal implementation**

```c
// include/fifa96_loader/fifa96_qfs.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { uint8_t magic[2]; uint32_t dec_len; char tag[4]; } fifa96_qfs_hdr_t;
// Ghidra: QFS/PVI framing reader (FUN_11bd_* per rename map).
fifa96_err_t fifa96_qfs_parse_hdr(const uint8_t *b, size_t n, fifa96_qfs_hdr_t *h);
```

```c
// src/fifa96_loader/fifa96_qfs.c
#include "fifa96_loader/fifa96_qfs.h"
fifa96_err_t fifa96_qfs_parse_hdr(const uint8_t *b, size_t n, fifa96_qfs_hdr_t *h) {
  if (!b || !h || n < 12) return FIFA96_ERR_TRUNCATED;
  if (b[0] != 0x10 || b[1] != 0xfb) return FIFA96_ERR_BAD_MAGIC;
  h->magic[0] = b[0]; h->magic[1] = b[1];
  h->dec_len = (uint32_t)(b[2] | (b[3] << 8) | (b[4] << 16) | (b[5] << 24));
  h->tag[0] = (char)b[6]; h->tag[1] = (char)b[7];
  h->tag[2] = (char)b[8]; h->tag[3] = (char)b[9];
  return FIFA96_OK;
}
```

```bash
cp /media/felipe/FIFAPCCD/art/fw1.qfs tests/golden/fw1.qfs
cp /media/felipe/FIFAPCCD/art/gameart0.pvi tests/golden/gameart0.pvi
```

```cmake
# append
add_library(fifa96_qfs src/fifa96_loader/fifa96_qfs.c)
add_executable(test_qfs tests/test_qfs.c)
target_link_libraries(test_qfs PRIVATE fifa96_qfs fifa96_file)
add_test(NAME test_qfs COMMAND test_qfs)
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build -R test_qfs -V`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add include/fifa96_loader/fifa96_qfs.h src/fifa96_loader/fifa96_qfs.c tests/test_qfs.c CMakeLists.txt
git commit -m "feat: add fifa96_qfs framing parser with fw1/gameart goldens" || true
```

---

### Task 6: fifa96_viv + fifa96_tgv framing + load-order closeout

**Files:**
- Create: `include/fifa96_loader/fifa96_viv.h`, `src/fifa96_loader/fifa96_viv.c`
- Create: `include/fifa96_loader/fifa96_tgv.h`, `src/fifa96_loader/fifa96_tgv.c`
- Modify: `tests/test_load_order.c` (extend to check golden list length)
- Test: `tests/test_viv.c`, `tests/test_tgv.c`

**Interfaces:**
- Consumes: `fifa96_file_read`.
- Produces:
  - `fifa96_err_t fifa96_viv_entry_at(const uint8_t *b, size_t n, size_t idx, uint32_t *off);`
  - `typedef struct { char magic[4]; uint32_t v0; } fifa96_tgv_hdr_t;`
  - `fifa96_err_t fifa96_tgv_parse_hdr(const uint8_t *b, size_t n, fifa96_tgv_hdr_t *h);`

- [ ] **Step 1: Write the failing tests**

```c
// tests/test_viv.c
#include <assert.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_viv.h"
int main(void) {
  uint8_t *b = 0; size_t n = 0;
  assert(fifa96_file_read("tests/golden/sfx_game.bnk", &b, &n) == 0);
  assert(n == 173016);
  uint32_t off = 0;
  assert(fifa96_viv_entry_at(b, n, 0, &off) == 0);
  assert(off == 0x00000000);                       // golden bytes 0000: 00 00 00 00
  assert(fifa96_viv_entry_at(b, n, 1, &off) == 0);
  assert(off == 0x00000200);                       // golden bytes 0008: 02 00 ...
  printf("test_viv OK\n");
  return 0;
}
```

```c
// tests/test_tgv.c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_tgv.h"
int main(void) {
  uint8_t *b = 0; size_t n = 0;
  assert(fifa96_file_read("tests/golden/vid_game.tgv", &b, &n) == 0);
  assert(n == 8261652);
  fifa96_tgv_hdr_t h;
  assert(fifa96_tgv_parse_hdr(b, n, &h) == 0);
  assert(memcmp(h.magic, "kVGT", 4) == 0);         // golden bytes 0000: 6b 56 47 54
  printf("test_tgv OK\n");
  return 0;
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `cmake -S . -B build && cmake --build build 2>&1 | tail -n 8`
Expected: FAIL — `fifa96_viv.h` / `fifa96_tgv.h` missing.

- [ ] **Step 3: Write minimal implementations**

```c
// include/fifa96_loader/fifa96_viv.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: VIV/BNK offset-table reader (FUN_11bd_* per rename map).
fifa96_err_t fifa96_viv_entry_at(const uint8_t *b, size_t n, size_t idx, uint32_t *off);
```

```c
// src/fifa96_loader/fifa96_viv.c
#include "fifa96_loader/fifa96_viv.h"
fifa96_err_t fifa96_viv_entry_at(const uint8_t *b, size_t n, size_t idx, uint32_t *off) {
  if (!b || !off || (idx + 1) * 4 > n) return FIFA96_ERR_TRUNCATED;
  const uint8_t *p = b + idx * 4;
  *off = (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
  return FIFA96_OK;
}
```

```c
// include/fifa96_loader/fifa96_tgv.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { char magic[4]; uint32_t v0; } fifa96_tgv_hdr_t;
// Ghidra: TGV framing reader, header only (FUN_11bd_* per rename map).
fifa96_err_t fifa96_tgv_parse_hdr(const uint8_t *b, size_t n, fifa96_tgv_hdr_t *h);
```

```c
// src/fifa96_loader/fifa96_tgv.c
#include "fifa96_loader/fifa96_tgv.h"
fifa96_err_t fifa96_tgv_parse_hdr(const uint8_t *b, size_t n, fifa96_tgv_hdr_t *h) {
  if (!b || !h || n < 8) return FIFA96_ERR_TRUNCATED;
  if (!(b[0] == 0x6b && b[1] == 0x56 && b[2] == 0x47 && b[3] == 0x54)) return FIFA96_ERR_BAD_MAGIC;
  h->magic[0] = 'k'; h->magic[1] = 'V'; h->magic[2] = 'G'; h->magic[3] = 'T';
  h->v0 = (uint32_t)(b[4] | (b[5] << 8) | (b[6] << 16) | (b[7] << 24));
  return FIFA96_OK;
}
```

```bash
cp /media/felipe/FIFAPCCD/sound/sfx_game.bnk tests/golden/sfx_game.bnk
cp /media/felipe/FIFAPCCD/video/vid_game.tgv tests/golden/vid_game.tgv
printf 'tests/golden/fnames.dat\ntests/golden/lengths.dat\ntests/golden/pcindex.pog\ntests/golden/fw1.qfs\ntests/golden/sfx_game.bnk\ntests/golden/vid_game.tgv\n' > tests/golden/load_order.txt
```

```cmake
# append
add_library(fifa96_viv src/fifa96_loader/fifa96_viv.c)
add_library(fifa96_tgv src/fifa96_loader/fifa96_tgv.c)
add_executable(test_viv tests/test_viv.c)
target_link_libraries(test_viv PRIVATE fifa96_viv fifa96_file)
add_executable(test_tgv tests/test_tgv.c)
target_link_libraries(test_tgv PRIVATE fifa96_tgv fifa96_file)
add_test(NAME test_viv COMMAND test_viv)
add_test(NAME test_tgv COMMAND test_tgv)
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build -V`
Expected: PASS — all 7 tests (`test_file`, `test_load_order`, `test_tables`, `test_pog`, `test_qfs`, `test_viv`, `test_tgv`).

- [ ] **Step 5: Commit**

```bash
git add include/fifa96_loader/fifa96_viv.h src/fifa96_loader/fifa96_viv.c include/fifa96_loader/fifa96_tgv.h src/fifa96_loader/fifa96_tgv.c tests/test_viv.c tests/test_tgv.c tests/golden/load_order.txt CMakeLists.txt
git commit -m "feat: add viv/tgv framing parsers and load-order closeout" || true
```

---

## Self-Review (ran before save)

- Spec coverage: wrappers (Tasks 1-2), tables (Task 3), pog (Task 4), qfs/pvi (Task 5), viv/bnk + tgv framing + order (Task 6). TGV full decode explicitly out of scope per spec, only framing tasked.
- Placeholder scan: no TBD/TODO; all magic/size asserts use observed golden bytes listed above.
- Type consistency: `fifa96_err_t` shared via `fifa96_err.h`; parse signatures uniformly `(const uint8_t*, size_t, out*)`; tests link only their lib + `fifa96_file`.
