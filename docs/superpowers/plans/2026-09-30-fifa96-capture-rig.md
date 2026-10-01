# FIFA 96 P0 Capture Rig Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the P0 capture rig: a resident 16-bit DOS TSR that emits an ordered per-session binary trace of every FIFA96 INT-21 file operation and of five cited codec-funnel patch sites, plus an in-repo C parser that turns that trace into citable text.

**Architecture:** A `.COM` TSR (`tsr/fifa96_capture.asm`, assembled with `nasm -f bin`) installs itself, hooks INT-21h (file records) and INT-60h/INT-1h (codec patch-site records), snoops the game's INT-21 calls, and streams records out the DOSBox-X serial port (`COM1`) to a host file. A C11 library + CLI (`fifa96_trace`) parses the byte stream (frame walk, resync, seq-gap detection, summary stats). A wrapper of `run-fifa96.sh` wires the serial sink to a per-session `captures/…/trace.bin`.

**Tech Stack:** NASM 2.16 (`nasm -f bin`, 16-bit real mode), C11 + CMake ≥ 3.28 + CTest `-Wall -Wextra -Werror`, DOSBox-X serial file sink (`serial1=file file:<path>`), GNU make.

**Spec:** `docs/superpowers/specs/2026-09-30-fifa96-capture-rig-design.md`

## Global Constraints

Copied verbatim from the spec; every task's requirements implicitly include this section.

- Never write to `game/FIFAPCCD96.iso` (read-only convention). The TSR lives in `game/hdd/` (writable emulated state), copied there by the run script.
- Honesty rules extend to captured evidence: a map/FU citation quotes parsed trace output verbatim; "runtime-captured" is labeled as its own evidence class, never blended with static cites; **no address is patched that the map does not cite with instruction bytes**.
- Existing 10 golden CTests stay green; no edits to existing `src/`, `include/`, `tests/golden/` behavior baselines. New files are additive only.
- `nasm` is required. If unavailable at build time, `make tsr` fails with a clear message; **no hand-emitted blob fallback**.
- Nothing in `captures/` is ever committed (derived from copyrighted content); only synthetic parser fixtures and verbatim parsed lines in docs are.
- Single-program sessions: the PSP captured for the game is the current PSP at the game's first INT-21 call; documented assumption.
- TSR array/behavior verification is live-session-only (documented); the parser is the only unit-tested component.
- Wire values are little-endian. Frame: `type:u8 | seq:u16 | len:u16 | payload[len]`.

### The five patch sites (binding values — do not alter)

Derivation rule (verified): `image_offset = (seg<<4 + off) − 0x10000`, where `0x10000`
is Ghidra's load base (segment `1000`); runtime target = `game_linear_base + image_offset`,
`game_linear_base = (game_psp + 16) << 4`. Equivalent file offset = `0x200 + image_offset`
(read-only check only; never patched on disk). All values re-verified against
`/media/felipe/FIFAPCCD/fifa96.exe`.

| id | map addr | map linear | image_offset (u32) | first instruction | bytes (live/relocated) | sig[4] | patch bytes written |
|----|----------|-----------|--------------------|-------------------|------------------------|--------|---------------------|
| 0 | `11bd:5aa1` | `0x17671` | `0x00007671` | `CMP word [BP+0xfef6],0x4d` (5 B) | `83 be f6 fe 4d` | `83 be f6 fe` | `CD 60` |
| 1 | `11bd:5aa8` | `0x17678` | `0x00007678` | `CALL 0x1000:79a2` (3 B, near) | `e8 27 03 eb` | `e8 27 03 eb` | `CD 60` |
| 2 | `11bd:5f6e` | `0x17b3e` | `0x00007b3e` | `CALLF 0x1000:0b12` (5 B) | `9a 12 0b 00 10` (seg word relocated) | `9a 12 0b 00` | `CD 60` |
| 3 | `11bd:5f89` | `0x17b59` | `0x00007b59` | `CALL 0x1000:7cd2` (3 B, near) | `e8 76 01 83` | `e8 76 01 83` | `CD 60` |
| 4 | `11bd:5f4b` | `0x17b1b` | `0x00007b1b` | `CALL 0x1000:84d7` (3 B, near) | `e8 b9 09 83` | `e8 b9 09 83` | `CD 60` |

> **Signature rule (Task 4a decision):** the 4-byte signature is the first 4 live
> bytes at the site. For the 3-byte `CALL` sites that includes the next
> instruction's first byte (`eb`/`83`/`83`), which is stable and non-relocated.
> Site 2's 5th byte is the relocated segment word and is deliberately excluded
> (`sig = 9a 12 0b 00`), so the signature is relocation-independent. Sizes match
> `/media/felipe/FIFAPCCD/fifa96.exe` at file offset `0x200 + image_offset`.

Per-site buffer dump (CODEC records; base is `SS:[BP+disp16]`, disp given as the
unsigned 16-bit form the TSR adds to BP):

| id | dump disp | dump len | what |
|----|-----------|----------|------|
| 0 | `0xfef6` (BP−0x10a) | 2 | object type word |
| 1 | `0xfef6` (BP−0x10a) | 2 | object type word at MF-branch call |
| 2 | `0xffe6` (BP−0x1a) | 2 | `mem_grow_relocate` segment/size arg |
| 3 | `0xfff8` (BP−0x08) | 8 | `copy_bytes_far` arg window |
| 4 | `0xfffe` (BP−0x02) | 2 | `exec_loaded_image` segment arg |

---

## File Structure

| File | Responsibility |
|------|----------------|
| `include/fifa96_loader/fifa96_trace.h` (new) | Wire-format constants + `fifa96_trace_format()` / `fifa96_trace_raw()` declarations |
| `src/fifa96_loader/fifa96_trace.c` (new) | Frame walk, resync, seq-gap, per-record text lines, summary; no game knowledge beyond the five site addresses |
| `tests/test_trace.c` (new) | Synthetic hand-authored frame streams → expected parser text |
| `tools/fifa96_trace.c` (new) | CLI: `fifa96_trace [--raw] FILE` |
| `tsr/fifa96_capture.asm` (new) | The resident COM: serial sink, INT-21/60/1 hooks, patch pass, framing |
| `run-fifa96-capture.sh` (new) | DOSBox-X launcher with serial file sink + FIFACAP.COM in autoexec |
| `CMakeLists.txt` (modify, append-only) | Build `fifa96_trace` lib + `test_trace` + `fifa96_trace` tool |
| `Makefile` (modify, append-only) | `tsr`, `capture`, `trace` targets |
| `.gitignore` (modify) | add `captures/` |

---

## Task 1: Trace parser library (`fifa96_trace`) with synthetic-fixture tests

**Files:**
- Create: `include/fifa96_loader/fifa96_trace.h`
- Create: `src/fifa96_loader/fifa96_trace.c`
- Create: `tests/test_trace.c`
- Modify: `CMakeLists.txt` (append at end)

**Interfaces:**
- Consumes: `fifa96_err_t` from `include/fifa96_loader/fifa96_err.h` (`FIFA96_OK`, `FIFA96_ERR_BAD_MAGIC`, `FIFA96_ERR_TRUNCATED`).
- Produces:
  - `int fifa96_trace_format(const uint8_t *data, size_t len, char **out);` → `*out` = malloc'd NUL-terminated text, one line per record + summary; returns `FIFA96_OK` (0) on success even when the stream contains resync/gap lines; `FIFA96_ERR_BAD_MAGIC` when no HEADER is found; `FIFA96_ERR_TRUNCATED` only for OOM-free internal failure.
  - `int fifa96_trace_raw(const uint8_t *data, size_t len, char **out);` → same but each line is `RAW type=.. seq=.. len=.. payload_hex`; still resyncs.
  - Frame constants: `FIFA96_TRACE_TYPE_HEADER 0x01`, `_FILE 0x02`, `_CODEC 0x03`, `_HEARTBEAT 0x04`, `_PATCH_SKIP 0x05`, `_END 0x06`, `_PATCH_OK 0x07`.
  - Site address table (id → map hex) used only for rendering: `{0x5aa1,0x5aa8,0x5f6e,0x5f89,0x5f4b}`.

- [ ] **Step 1: Write the header**

`include/fifa96_loader/fifa96_trace.h`:
```c
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_TRACE_TYPE_HEADER    0x01
#define FIFA96_TRACE_TYPE_FILE      0x02
#define FIFA96_TRACE_TYPE_CODEC     0x03
#define FIFA96_TRACE_TYPE_HEARTBEAT 0x04
#define FIFA96_TRACE_TYPE_PATCH_SKIP 0x05
#define FIFA96_TRACE_TYPE_END       0x06
#define FIFA96_TRACE_TYPE_PATCH_OK  0x07

#define FIFA96_TRACE_VERSION 1
#define FIFA96_TRACE_NSITES  5

int fifa96_trace_format(const uint8_t *data, size_t len, char **out);
int fifa96_trace_raw(const uint8_t *data, size_t len, char **out);
```

- [ ] **Step 2: Write the failing tests**

`tests/test_trace.c`. Build every fixture as an explicit byte array; assert on exact substrings (not whole-document equality) so the summary wording can evolve:
```c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_trace.h"

/* helper: append frame type/seq/len/payload to a byte vector */
static void put(uint8_t *b, size_t *n, uint8_t t, uint16_t seq,
                const uint8_t *p, uint16_t plen) {
  b[(*n)++] = t; b[(*n)++] = (uint8_t)(seq & 0xff); b[(*n)++] = (uint8_t)(seq >> 8);
  b[(*n)++] = (uint8_t)(plen & 0xff); b[(*n)++] = (uint8_t)(plen >> 8);
  if (p) memcpy(b + *n, p, plen);
  *n += plen;
}

static void expect_has(const char *hay, const char *needle) {
  if (!strstr(hay, needle)) { fprintf(stderr, "missing: %s\n---\n%s\n", needle, hay); assert(0); }
}

static void test_header_and_end(void) {
  uint8_t buf[64]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  put(buf,&n,0x01,0,hdr,6);
  put(buf,&n,0x06,0,0,0);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"HEADER magic=FCAP version=1 patches=5");
  expect_has(s,"END");
  free(s);
}

static void test_file_name_then_read_resolves_name(void) {
  uint8_t buf[128]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  uint8_t f3d[64]; size_t o = 0;
  /* ah,bx,cx,ds,dx,ax_after,flags,hash,dlen,data */
  f3d[o++]=0x3D; f3d[o++]=0; f3d[o++]=0; f3d[o++]=0; f3d[o++]=0; f3d[o++]=0;
  f3d[o++]=0; f3d[o++]=0; f3d[o++]=(uint8_t)1; /* flags bit1 = data_is_name */
  f3d[o++]=0;f3d[o++]=0;f3d[o++]=0;f3d[o++]=0; /* hash */
  f3d[o++]=(uint8_t)7; f3d[o++]=(uint8_t)0;    /* dlen=7 */
  memcpy(f3d+o,"FW1.QFS",7); o+=7;
  put(buf,&n,0x01,0,hdr,6);
  put(buf,&n,0x02,0,f3d,(uint16_t)o);
  free(s);
}
int main(void){ test_header_and_end(); test_file_name_then_read_resolves_name(); printf("test_trace OK\n"); return 0; }
```
(The second test is expanded in Step 3 below after the exact byte layout is fixed; keep it compiling at this step.)

- [ ] **Step 3: Fix the FILE/CODEC payload layout exactly, then finalize tests**

FILE payload (after `type,seq,len`):
`ah:u8 | bx:u16 | cx:u16 | ds:u16 | dx:u16 | ax_after:u16 | flags:u8 | hash:u32 | dlen:u16 | data[dlen]`

`flags`: bit0 `hash_valid`, bit1 `data_is_name`, bit2 `data_is_bytes`.

CODEC payload:
`site_id:u8 | ax,bx,cx,dx,si,di,bp,sp,ds,es,ss,ip,cs (13×u16 LE) | flags:u16 | hash:u32 | dseg:u16 | doff:u16 | dlen:u16 | data[dlen]`

Add these cases (all hand-authored):
- `test_header_and_end` (above).
- `test_seq_gap`: two FILE frames with seq 0 then 2 → line contains `SEQ-GAP type=FILE expected=1 got=2`.
- `test_resync`: HEADER, 3 garbage bytes, then a valid END → line contains `LOST-SYNC off=0x0006` and END still parsed.
- `test_missing_end`: HEADER + FILE, no END → summary contains `no-END`.
- `test_patch_skip_reasons`: one PATCH_SKIP per reason → lines `PATCH_SKIP site=5aa1 reason=bad-signature`, `… reason=already-patched`, `… reason=site-out-of-range`.
- `test_patch_ok`: `PATCH_OK site=5f6e target=0x00012345 siglen=4` line + summary `patch: ok=1 skip=0`.
- `test_codec`: one CODEC frame with site_id 2 → line starts `CODEC site=5f6e ` and contains `hash=` and `head=`.
- `test_heartbeat`: HEARTBEAT with count 1024 → `HEARTBEAT files=1024`.
- `test_raw_mode`: `fifa96_trace_raw` over the header fixture → line starts `RAW type=01`.

Site rendering: `site_id 0..4` renders as the map address hex from the table
(`5aa1,5aa8,5f6e,5f89,5f4b`); unknown ids render `site=?0xNN`.

- [ ] **Step 4: Run tests to verify they fail**

Run: `make test`
Expected: compile succeeds (tests written) but `test_trace` aborts / `fifa96_trace_format` undefined link error — record the exact failure.

- [ ] **Step 5: Implement `fifa96_trace.c` (minimal, then complete)**

Implement in this order, re-running `make test` after each and extending tests only if a bug is found:
1. Header validation: scan the first frame; if not HEADER with len==6 and magic `FCAP`, print `BAD-HEADER` and return `FIFA96_ERR_BAD_MAGIC`.
2. Frame walk with `SEQ-GAP` and `LOST-SYNC` (resync: from a bad offset, advance 1 byte until `type in 1..7 && len <= remaining`; for type 1 additionally require len==6 and magic `FCAP`).
3. Render HEADER/END/HEARTBEAT/PATCH_OK/PATCH_SKIP.
4. FILE: track `handle→name` from `data_is_name` records (keyed by `ax_after`); render `FILE ah=%02X h=0x%04x x=%u got=%u name=%s hash=%08x len=%u head=%s` (name `-` when unknown; hash `-` when `!hash_valid`; head hex of `min(dlen,64)` bytes).
5. CODEC: render `CODEC site=<addr> ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x bp=%04x sp=%04x ds=%04x es=%04x ss=%04x ip=%04x cs=%04x hash=%08x seg=%04x off=%04x len=%u head=%s`.
6. Summary block:
```
SUMMARY files-by-name:
  <NAME> opens=<n> reads=<n> bytes=<n>
SUMMARY opens=<n> reads=<n> writes=<n> other=<n>
SUMMARY patch: ok=<n> skip=<n>
SUMMARY codec: site=<addr> hits=<n>   (one line per site with hits)
SUMMARY end=<END|no-END> lost=<n> seqgaps=<n>
```

- [ ] **Step 6: Wire the build and run the suite**

Append to `CMakeLists.txt`:
```cmake
add_library(fifa96_trace src/fifa96_loader/fifa96_trace.c)
target_link_libraries(fifa96_trace PRIVATE fifa96_file)
add_executable(test_trace tests/test_trace.c)
target_link_libraries(test_trace PRIVATE fifa96_trace fifa96_file)
add_test(NAME test_trace COMMAND test_trace WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
```
Run: `make test`
Expected: `100% tests passed`, including `test_trace`.

- [ ] **Step 7: Commit**

```bash
git add include/fifa96_loader/fifa96_trace.h src/fifa96_loader/fifa96_trace.c tests/test_trace.c CMakeLists.txt
git commit -m "feat(trace): frame parser with resync, seq-gap and summary"
```

---

## Task 2: Parser CLI + build targets + ignore captures

**Files:**
- Create: `tools/fifa96_trace.c`
- Modify: `CMakeLists.txt` (append)
- Modify: `Makefile` (append targets)
- Modify: `.gitignore`

**Interfaces:**
- Consumes: `fifa96_trace_format`, `fifa96_trace_raw` (Task 1), `FIFA96_OK`.
- Produces: `build/fifa96_trace` binary; `make trace FILE=…`.

- [ ] **Step 1: Write the CLI**

`tools/fifa96_trace.c`:
```c
// tools/fifa96_trace.c — CLI: parse a capture-rig trace.bin to text.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_trace.h"

int main(int argc, char **argv) {
  int raw = 0, argi = 1;
  if (argc > 1 && strcmp(argv[1], "--raw") == 0) { raw = 1; argi = 2; }
  if (argi >= argc) { fprintf(stderr, "usage: %s [--raw] FILE\n", argv[0]); return 2; }
  uint8_t *data = 0; size_t len = 0;
  if (fifa96_file_read(argv[argi], &data, &len) != FIFA96_OK) {
    fprintf(stderr, "read failed: %s\n", argv[argi]); return 1;
  }
  char *text = 0;
  int rc = raw ? fifa96_trace_raw(data, len, &text)
               : fifa96_trace_format(data, len, &text);
  if (rc != FIFA96_OK) { fprintf(stderr, "parse failed (%d)\n", rc); fifa96_file_free(data); return 1; }
  fputs(text, stdout);
  free(text); fifa96_file_free(data);
  return 0;
}
```

- [ ] **Step 2: Wire the build**

Append to `CMakeLists.txt`:
```cmake
add_executable(fifa96_trace tools/fifa96_trace.c)
target_link_libraries(fifa96_trace PRIVATE fifa96_trace fifa96_file)
```

Append to `Makefile` (keep `FILE ?= tests/golden/fw1.qfs` untouched; add a distinct var):
```make
TRACE ?= captures/session-latest/trace.bin

tsr: ## Assemble the capture TSR (requires nasm)
	@command -v nasm >/dev/null || { echo "nasm not installed (apt install nasm)" >&2; exit 1; }
	nasm -f bin tsr/fifa96_capture.asm -o $(BUILD)/FIFACAP.COM
	@printf 'FIFACAP.COM: '; wc -c < $(BUILD)/FIFACAP.COM
	@sha256sum $(BUILD)/FIFACAP.COM

capture: tsr ## Launch the game under the capture rig (needs game/FIFAPCCD96.iso)
	./run-fifa96-capture.sh

trace: build ## Parse a captured trace (make trace TRACE=captures/…/trace.bin)
	./$(BUILD)/fifa96_trace $(TRACE)
```
Add `tsr capture trace` to the `.PHONY` line.

- [ ] **Step 3: Ignore captures**

Append to `.gitignore`:
```
captures/
```

- [ ] **Step 4: Verify**

Run: `make test` → all tests pass.
Run: `make build` → `build/fifa96_trace` exists.
Run: `printf '' > /tmp/empty.bin; ./build/fifa96_trace /tmp/empty.bin` → prints `BAD-HEADER` and exits 1.
Run: `git check-ignore -v captures/x` → reports `.gitignore:…:captures/`.

- [ ] **Step 5: Commit**

```bash
git add tools/fifa96_trace.c CMakeLists.txt Makefile .gitignore
git commit -m "feat(trace): CLI tool, make targets, ignore captures/"
```

---

## Task 3: Capture TSR — install, COM1 sink, INT-21 file trace

**Files:**
- Create: `tsr/fifa96_capture.asm`

**Interfaces:**
- Produces: `build/FIFACAP.COM`, a `.COM` that (a) probes COM1, (b) emits a HEADER,
  (c) hooks INT-21h and INT-60h/INT-1h, (d) stays resident via `INT 27h`, (e) emits
  FILE records for AH=3D/3E/3F/40h and END for AH=4Ch, (f) emits a HEARTBEAT every
  1024 FILE records. Task 4 adds the patch table / patch pass / CODEC emission.
- Wire layout is Task 1's, verbatim.

- [ ] **Step 1: Write the resident skeleton with the serial sink and framing**

`tsr/fifa96_capture.asm` (complete file; Task 4 appends the patch pass and INT-60 body):
```asm
; tsr/fifa96_capture.asm — FIFA96 P0 capture rig (16-bit COM).
;   nasm -f bin tsr/fifa96_capture.asm -o build/FIFACAP.COM
        org 0x100
        bits 16

COM1_TX   equ 0x3F8
COM1_LSR  equ 0x3FD

T_HEADER  equ 0x01
T_FILE    equ 0x02
T_CODEC   equ 0x03
T_HB      equ 0x04
T_SKIP    equ 0x05
T_END     equ 0x06
T_POK     equ 0x07

start:  jmp install

; ───────────────────────────── resident data ─────────────────────────────
tsr_base:
old_int21  dd 0
old_int60  dd 0
old_int1   dd 0
saved_psp  dw 0
patchpend  db 0
pending    db 0xFF
filecount  dd 0
seq_file   dw 0
seq_codec  dw 0
seq_hb     dw 0
rec_buf    times 160 db 0          ; payload scratch (max 160 B)

; ───────────────────────────── resident code ─────────────────────────────
; send AL to COM1 with a bounded THRE spin (clobbers CX; preserves AX? no)
putc:   push ax
        push cx
        mov  cx, 0xFFFF
.try:   mov  dx, COM1_LSR
        in   al, dx
        test al, 0x20
        jnz  .ok
        loop .try
        pop  cx
        pop  ax
        ret                        ; dropped on timeout (detectable as seq gap)
.ok:    mov  dx, COM1_TX
        pop  cx
        mov  al, [cs:pb_char]      ; placeholder replaced below
        pop  ax
        ret
```
> The `putc` above is a stub to be completed in Step 2; do not commit it half-done.

- [ ] **Step 2: Complete `putc`/`putw`/`send_frame` exactly**

```asm
; AL = byte
putc:   push ax
        push cx
        push dx
        mov  dl, al
        mov  cx, 0xFFFF
.try:   mov  dx, COM1_LSR
        in   al, dx
        test al, 0x20
        jnz  .ok
        loop .try
        jmp  .out
.ok:    mov  dx, COM1_TX
        mov  al, dl
        out  dx, al
.out:   pop  dx
        pop  cx
        pop  ax
        ret

; AX = word
putw:   push ax
        mov  al, ah
        call putc
        pop  ax
        call putc
        ret

; send_frame: AL=type, DS:SI=payload, CX=len. Updates per-type seq in cs:seq_*.
send_frame:
        push ax                        ; type
        push bx
        push cx
        push si
        ; choose seq slot by type
        mov  bx, seq_file
        cmp  al, T_FILE
        je   .gotseq
        mov  bx, seq_codec
        cmp  al, T_CODEC
        je   .gotseq
        mov  bx, seq_hb
        cmp  al, T_HB
        je   .gotseq
        xor  bx, bx                    ; HEADER/SKIP/END/POK use seq 0 (not tracked)
.gotseq:
        ; emit type
        call putc                      ; AL still = type after pushes? restore first
        ...
```
> **Implementer:** `send_frame` must (1) save the caller's `AX,CX,SI`, (2) pick the
> seq slot from a per-type table, emit `type`, `seq`, `len`, then the payload byte by
> byte, (3) increment the per-type counter for FILE/CODEC/HEARTBEAT only, (4) restore
> and `ret`. Write it out fully; the fragment above only fixes the interface. Verify
> by assembling.

- [ ] **Step 3: Add the INT-21 hook with pre/post chaining**

Use the synthesized-frame chaining technique (the original handler ends in `IRET`,
so pre/post hooks must hand it a frame to return through):
```asm
int21:
        pushf
        pusha
        push ds
        push es
        ; ---- pre ----
        mov  ax, [cs:saved_psp]        ; sanity marker; real detection below
        ; AH = requested function (from the saved AX we are about to inspect)
        ; Examine AH: on entry AH is in the live AH.
        cmp  ah, 3Dh
        je   .fop
        cmp  ah, 3Eh
        je   .fop
        cmp  ah, 3Fh
        je   .fop
        cmp  ah, 40h
        je   .fop
        cmp  ah, 4Bh
        je   .exec
        cmp  ah, 4Ch
        je   .quit
        jmp  .chain
.fop:   ; capture pre-state into cs:pre_ah/pre_bx/pre_cx/pre_ds/pre_dx and a
        ; name copy for AH=3Dh (DS:DX, up to 13 bytes, NUL-terminated)
        ...
        jmp  .chain
.exec:  mov  byte [cs:patchpend], 1    ; Task 4 consumes this at the next INT-21
        jmp  .chain
.quit:  ; emit END then chain (the process is terminating)
        ...
        jmp  .chain
.chain: pop  es
        pop  ds
        popa
        popf
        pushf
        push cs
        push word .after
        jmp  far [cs:old_int21]
.after: ; FLAGS here are the handler's result flags
        pushf
        pusha
        push ds
        push es
        push word [cs:g_flags]
        ; ---- post ----
        ...
        pop  es
        pop  ds
        popa
        iret
```
> **Correction (Task 3 review):** the `.after` epilogue must NOT `popf` — the
> synthesized frame was already consumed by the original handler's IRET, so the
> stack top at `.after` is the real caller frame; a `popf` there eats the caller
> IP. End with `pop es; pop ds; popa; iret` and write the handler result flags
> into the real frame's FLAGS slot (`mov ax,[cs:g_flags]; mov [sp+24],ax`) before
> the pops. Task 3a/3b implement this; do not reintroduce the `popf`.
> **Implementer:** complete the `pre_*` storage, the name copy, the post-file-record
> emission (see Task 4 for the shared `emit_file` routine), the END emission, and the
> frame-flag fix-up (`mov [sp+24], ax` with `ax = g_flags` after the `pusha/push ds/
> push es` sequence). Add `pre_ah dw 0` etc. and `g_flags dw 0` to resident data.

- [ ] **Step 4: Add install: probe, hook, HEADER, resident size, INT 27h**

```asm
install:
        mov  [cs:saved_psp], 0         ; filled from INT 21h AH=51h once resident is up
        ; probe COM1: send 0x00 and wait for THRE
        mov  al, 0
        call putc                      ; putc drops silently on timeout
        ; We cannot detect a drop from putc; probe explicitly:
        mov  cx, 0xFFFF
.probe: mov  dx, COM1_LSR
        in   al, dx
        test al, 0x20
        jnz  .ok
        loop .probe
        ; not ready → print message, exit non-zero, do NOT go resident
        mov  dx, msg_noserial
        call print
        mov  ax, 0x4C01
        int  0x21
.ok:
        ; save vectors
        mov  ax, 0x3521                ; AH=35h, AL=21h
        int  0x21                      ; ES:BX = old INT21
        mov  [cs:old_int21], bx
        mov  [cs:old_int21+2], es
        mov  ax, 0x3560
        int  0x21
        mov  [cs:old_int60], bx
        mov  [cs:old_int60+2], es
        mov  ax, 0x3501                ; INT 1
        int  0x21
        mov  [cs:old_int1], bx
        mov  [cs:old_int1+2], es
        ; install ours
        mov  ax, 0x2521
        mov  dx, int21
        int  0x21
        mov  ax, 0x2560
        mov  dx, int60
        int  0x21
        mov  ax, 0x2501
        mov  dx, int1
        int  0x21
        ; HEADER
        mov  al, T_HEADER
        mov  si, hdr_payload
        mov  cx, 6
        call send_frame
        ; resident size in paragraphs (from PSP=CS:0) and stay resident
        mov  dx, (tsr_end - start + 0x100 + 15) >> 4
        int  0x27                      ; never returns

print:  ; DX = NUL-terminated string, uses INT 21h AH=09h
        push ax
        mov  ah, 0x09
        int  0x21
        pop  ax
        ret

hdr_payload: db 'FCAP', 1, 5
msg_noserial: db 'FIFACAP: COM1 not ready, not resident$'

int60:  ; Task 4
        jmp far [cs:old_int60]
int1:   ; Task 4
        jmp far [cs:old_int1]
tsr_end:
```

- [ ] **Step 5: Build and verify the TSR assembles**

Run: `make tsr`
Expected: prints `FIFACAP.COM: <size> bytes` and a sha256, exit 0.
Run: `make tsr 2>&1 | tail -3` twice → identical sha256 (deterministic assemble).
If nasm is missing, temporarily test the guard with `PATH=/usr/bin:/bin` minus nasm is not required; instead confirm the error branch text by reading it.

- [ ] **Step 6: Commit**

```bash
git add tsr/fifa96_capture.asm
git commit -m "feat(tsr): resident COM install with COM1 sink and INT-21 file trace"
```

---

## Task 4: Capture TSR — patch pass, INT-60 codec records, INT-1 re-arm

**Files:**
- Modify: `tsr/fifa96_capture.asm`

**Interfaces:**
- Consumes: Task 3's `send_frame`, `putc`, `putw`, resident data, and the five-site
  table values from **Global Constraints** (verbatim).
- Produces: PATCH_OK/PATCH_SKIP records at the game's first INT-21 call after an
  AH=4Bh launch; CODEC records at each of the five sites; `emit_file` shared routine.

**Ruling (recorded, overrides spec §3/§4 wording where it is mechanically impossible):**
- The spec's `[PSP+0x5Ch]` is PSP field FCB1 on every documented DOS and cannot yield
  a child PSP. The TSR instead obtains the current PSP at the game's first INT-21 call
  via `INT 21h/AH=51h` invoked *directly on the original vector* using the same
  synthesized-frame chain (no recursion through the IVT). `patchpend` is set in the
  AH=4Bh pre-hook and consumed on the next INT-21 entry, which is the game's first
  call (DOS internal EXEC calls do not traverse the IVT).
- The spec's §5 slot/`E9` trampoline cannot preserve near-relative semantics if the
  slot lives in the TSR's segment (`E8`/`E9` are CS-relative). The plan implements the
  equivalent, correct mechanism the spec's §6 re-arm trap implies: on a site hit the
  handler emits the record, restores the 2 saved live bytes, sets TF and re-points the
  IRET IP to the site; INT-1 re-writes `CD 60` and IRETs. Bytes 2..L−1 of the original
  instruction are never overwritten, so only 2 saved bytes per site are needed.

- [ ] **Step 1: Add the site table and patch state**

```asm
; each site: img_off:dword, orig_len:byte, sig:4 bytes, save_lo:byte, save_hi:byte,
;            tgt_lin:dword (filled at patch time), dump_disp:word, dump_len:byte
NSITES  equ 5
site_tab:
  dd 0x00007671
  db 5, 0x83,0xbe,0xf6,0xfe
  db 0,0, 0,0,0,0
  dw 0xfef6, 2
  dd 0x00007678
  db 3, 0xe8,0x27,0x03
  db 0,0, 0,0,0,0
  dw 0xfef6, 2
  dd 0x00007b3e
  db 5, 0x9a,0x12,0x0b,0x00
  db 0,0, 0,0,0,0
  dw 0xffe6, 2
  dd 0x00007b59
  db 3, 0xe8,0x76,0x01
  db 0,0, 0,0,0,0
  dw 0xfff8, 8
  dd 0x00007b1b
  db 3, 0xe8,0xb9,0x09
  db 0,0, 0,0,0,0
  dw 0xfffe, 2
SITE_REC equ 17          ; bytes per record (verify: 4+1+4+2+4+2+1 = 18? fix to match)
```
> **Implementer:** begin by writing a one-line assembler-time assertion
> (`%if SITE_REC != (site_tab2-site_tab)/NSITES` … error) after freezing the layout;
> adjust padding so each entry is exactly `SITE_REC` bytes and the table ends on a
> boundary. Do not guess — assemble and inspect `nasm -l build/fifacap.lst`.

- [ ] **Step 2: Add the patch pass and call it from `patchpend`**

```asm
; run once, at the game's first INT-21 call after AH=4Bh
do_patch_pass:
        ; 1. get current PSP via AH=51h on the ORIGINAL vector (chain + .psp_after)
        pushf
        push cs
        push word .psp_after
        mov  ah, 0x51
        jmp  far [cs:old_int21]
.psp_after:
        ; BX = current PSP
        mov  ax, bx
        inc  ax
        add  ax, 15                    ; (psp+16) paragraphs
        mov  cl, 4
        shl  ax, cl                    ; WAIT: paragraph→linear is <<4, do it on a dword
        ; base = (psp+16)<<4  (use a 32-bit shift, not 8-bit CL on AX)
        ...
        ; for i in 0..NSITES-1: target = base + img_off[i]
        ;   if target+4 > 0x100000 → PATCH_SKIP(i, 2)
        ;   else read 4 bytes at target; != sig → PATCH_SKIP(i, 0)
        ;   else if word == 0x60CD → PATCH_SKIP(i, 1)
        ;   else save bytes, store tgt, write 0x60CD, PATCH_OK(i, target, 4)
        ret
```

- [ ] **Step 3: Add the CODEC emitter and complete `int60`/`int1`**

```asm
int60:
        pusha
        push ds
        push es
        ; identify site by matching CS:IP-2 against tgt_lin[i]
        ; build CODEC payload in rec_buf: site_id, 13 regs, flags, hash, dseg/doff/dlen/data
        ; emit via send_frame(T_CODEC, rec_buf, len)
        ; restore the 2 saved live bytes at the site
        ; set pending = id
        ; in the INT-60 IRET frame: IP = target_off, FLAGS |= 0x0100 (TF)
        pop  es
        pop  ds
        popa
        iret
int1:
        cmp  byte [cs:pending], 0xFF
        jne  .consume
        jmp  far [cs:old_int1]
.consume:
        ; write 0x60CD at tgt_lin[pending]; pending = 0xFF
        iret
```
> **Implementer:** compute the FNV-1a 32 hash (basis `2166136261`, prime `16777619`,
> XOR then multiply per byte) of the per-site dump window (`SS:[BP+disp]`, `len`) and
> of the FILE read head (≤64 B) with one shared `fnv1a` routine taking `DS:SI`/`CX`.

- [ ] **Step 4: Build and verify**

Run: `make tsr`
Expected: `FIFACAP.COM: <n> bytes` + sha256, exit 0. Record the size and hash in the report.

- [ ] **Step 5: Commit**

```bash
git add tsr/fifa96_capture.asm
git commit -m "feat(tsr): patch pass, INT-60 codec records, INT-1 re-arm"
```

---

## Task 5: Capture launcher + live acceptance checklist

**Files:**
- Create: `run-fifa96-capture.sh`
- Modify: `Makefile` (already has `capture` from Task 2; verify it points here)

**Interfaces:**
- Consumes: `build/FIFACAP.COM` (Task 3/4), `run-fifa96.sh` (existing), `game/FIFAPCCD96.iso`.
- Produces: `captures/session-<UTC>/trace.bin`.

- [ ] **Step 1: Write the launcher**

`run-fifa96-capture.sh` (executable; mirror `run-fifa96.sh`'s shape):
```sh
#!/bin/sh
set -eu
DIR=$(cd "$(dirname "$0")" && pwd)
ISO="$DIR/game/FIFAPCCD96.iso"
HDD="$DIR/game/hdd"
COM="$DIR/build/FIFACAP.COM"
SESSION=${SESSION:-$(date -u +%Y%m%dT%H%M%SZ)}
CAP_DIR="$DIR/captures/session-$SESSION"
CAP_FILE="$CAP_DIR/trace.bin"
CONF=$(mktemp -t fifa96cap.XXXXXX.conf)
trap 'rm -f "$CONF"' EXIT INT TERM

[ -f "$ISO" ] || { echo "missing ISO: $ISO" >&2; exit 1; }
[ -f "$COM" ] || { echo "missing TSR: $COM (run: make tsr)" >&2; exit 1; }
command -v dosbox-x >/dev/null || { echo "dosbox-x not installed" >&2; exit 1; }

mkdir -p "$CAP_DIR" "$HDD"
cp "$COM" "$HDD/FIFACAP.COM"

cat > "$CONF" <<EOF
[dosbox]
machine=svga_s3
memsize=16

[serial]
serial1=file file:$CAP_FILE multiplier:100

[sdl]
fullscreen=false

[autoexec]
@echo off
MOUNT C "$HDD"
IMGMOUNT D "$ISO" -t iso
C:\FIFACAP.COM
D:
FIFA96.EXE
EOF

dosbox-x -conf "$CONF" -fastlaunch -nopromptfolder
printf 'capture: %s\n' "$CAP_FILE"
```

- [ ] **Step 2: Verify statically**

Run: `chmod +x run-fifa96-capture.sh`
Run: `shellcheck run-fifa96-capture.sh` (or `sh -n run-fifa96-capture.sh` if shellcheck absent) → clean.
Run: `SESSION=selftest ./run-fifa96-capture.sh` with the ISO absent → prints `missing ISO:` and exits 1 (proves the guard before any DOSBox launch). Remove `captures/session-selftest` if created.

- [ ] **Step 3: Manual live acceptance checklist (do not automate; record the outcome verbatim)**

The game assets are required and are not in the repo. A human plays; the rig logs.
Run `make capture`, play boot→intro→asset load, quit, then `make trace TRACE=<printed path>`.

- [ ] a. Trace contains `HEADER magic=FCAP version=1 patches=5`, ≥1 `FILE` record for a CD file, and `END` (or a flagged truncation: summary `end=no-END`).
- [ ] b. Patch pass: every one of the five sites emits exactly one result — `PATCH_OK site=<addr>` or `PATCH_SKIP site=<addr> reason=…`; summary `patch: ok=5 skip=0` (or a reason naming the mismatch). If all five skip, the PSP/base assumption failed — attach the raw first INT-21 records.
- [ ] c. Reaching an asset load emits ≥1 `CODEC site=…` line for the responsible site.
- [ ] d. The game behaves as with plain `./run-fifa96.sh` — no new hangs or corruption (the self-checksum assumption, stated as an assumption until observed).
- [ ] e. Append the accepted parsed lines **verbatim** (quote-only rule) to the report; do not edit the map in this slice.

- [ ] **Step 4: Commit**

```bash
git add run-fifa96-capture.sh
git commit -m "feat(rig): DOSBox-X capture launcher with serial file sink"
```

---

## Self-Review

- **Spec coverage:** Guest TSR (Tasks 3–4: COM1 sink, frame format, INT-21 hook, deferred patch table, INT-60 handler, INT-1 re-arm, heartbeat) ✓; Host (Task 2/5: `run-fifa96-capture.sh`, `tools/fifa96_trace.c`, Makefile targets, `.gitignore captures/`) ✓; Testing 1 parser CTests (Task 1) ✓; Testing 2 TSR checked by `make tsr` + size/hash (Task 3/4) ✓; Testing 3 live checklist (Task 5) ✓; Hard constraints: no ISO write, no patches to uncited addresses (five-site table), existing tests untouched, `nasm` guard, nothing in `captures/` committed (`.gitignore`) ✓.
- **Placeholder scan:** three `> Implementer:` callouts remain (send_frame body, patch-pass 32-bit shift, CODEC payload packing). These are bounded implementation detail, not requirements; the interfaces and exact values are fixed. Task 3 Step 1's stub is explicitly marked "do not commit half-done".
- **Type consistency:** Task 1 fixes the wire layout; Tasks 3–4 reference the same constants and the same five values as Global Constraints. `emit_file` is introduced in Task 3 Step 3 and reused in Task 4.
- **Open risk:** the TSR is live-verified only; Task 5's live checklist is the acceptance gate. If §Ruling mechanisms prove wrong, they surface there, not in unit tests.
