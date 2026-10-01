# FIFA96 DPMI Trace Probe Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Capture the game-side caller chain at four resource entry points (`FUN_0009e718` decompressor, `FUN_0009e860` decode-call, `FUN_000c9d10`/`FUN_000cad30` loaders) by executing a protected-mode trampoline that reports through DPMI `INT 31h AX=0300h` to a resident serial TSR — no DOSBox-X debugger required.

**Architecture:** A 32-bit cave injected at the LE image's only in-range zero area (link `0x6728D`) is entered by a `call rel32` patched over each target's entry. The cave reads the caller return address from the stack, builds a DPMI real-mode register structure on its own stack, and simulates `INT 61h`; a new TSR handler (`T_PROBE = 0x08`) dumps the passed registers to COM1 in the existing frame format. A Python tool extracts `FIFA96.EXE` from a copy of the ISO, applies trampolines at LE file offsets, and the existing capture script runs the patched copy headless; a decoder recovers caller return addresses, derives the runtime delta per run (`target_ret − (target + overwrite)`), and normalizes to link addresses for cross-checking against the static caller lists in FU-8/FU-9.

**Tech Stack:** nasm + ndisasm (installed), Python 3 `unittest` under CTest, DOSBox-X headless (`-silent`) serial capture rig (`tsr/fifa96_capture.asm`), `tools/fifa96_le.py` LE parser.

**Spec:** `docs/ghidra/FU10_dpmi_trace_plan.md` (approved plan) with evidence from `docs/ghidra/FU8_loader_chain.md`, `docs/ghidra/FU9_format_consumers.md`, and the `/tmp/opencode/daytrace` experiments (PM cave precedent, ISO in-place patch precedent, `dosbox-x-nocap` host-scan rig).

## Global Constraints

- All 14 existing CTest tests must stay green (`make test`); only ADD tests. `src/`, `include/`, existing test expectations, and `CMakeLists.txt` test semantics for existing tests are NOT changed.
- **Never write to `game/FIFAPCCD96.iso` or `/media/felipe/FIFAPCCD/`.** Every patched artifact is a copy under `build/` (`build/` is gitignored). The tool reads the input ISO only.
- No Ghidra program writes. Static caller lists for cross-check come from FU-8/FU-9 verbatim; the probe measures the runtime lists.
- The runtime linear delta is MEASURED each run (`delta = target_ret − (target_link + overwrite)`); never hardcode `0x2D1000` in a decoder/assertion — it is a prior hypothesis, the probe is the authority.
- `captures/` is gitignored: docs quote only verbatim decoded lines with session provenance (`captures/session-<name>/trace.bin`).
- Capture-rig invariants preserved in the TSR: `INT 21h AH=31h` residency, COM1 divisor-1 programming, forward only CF from chained handler results.
- The cave must be placed only in the obj1 zero cave at link `0x6728D` (capacity `0x103` bytes); links outside stored pages are rejected.
- Commit messages exactly as each task brief specifies; no `fifa96.rep/**` staging; no `git push`.

---

## Scope Check

Single subsystem: the trace probe (proto). Four files of tooling plus TSR/script changes; no loader C changes. If the DPMI path is unsupported by DOS4GW, Task 4 reports BLOCKED with decoder evidence and the controller rules on the fallback (the `INT 21h` filename channel through the existing TSR is the named fallback, not implemented unless ruled in).

## File Structure

- Create `tools/fifa96_probe.py` — probe-frame decoder + delta normalization + CLI (Task 1).
- Create `tests/test_probe.py` — decoder unit tests (Task 1); registered in CMake Python block.
- Modify `tsr/fifa96_capture.asm` — `T_PROBE` + `int61` handler + install hooks (Task 2).
- Create `tsr/probe_stub.asm` — 16-bit real-mode `INT 61h` smoke stub (Task 2).
- Create `tools/int61_smoke.sh` — headless DOSBox smoke: TSR + stub → serial → decoder assert (Task 2).
- Create `tools/fifa96_patch.py` — ISO/LE trampoline injector + cave assembler (Task 3).
- Create `tests/test_patch.py` — injector unit tests (Task 3); registered in CMake Python block.
- Modify `run-fifa96-capture.sh` — `ISO` override + `HEADLESS` + `TIMEOUT` env (Task 3), default behavior unchanged.
- Create `tools/trace_probe.sh` — one-command target run: patch ISO copy → headless capture → decode (Task 4).
- Create `docs/ghidra/FU11_dpmi_probe_findings.md` — findings (Tasks 4, 5).

---

### Task 1: Probe-frame decoder

**Files:**
- Create: `tools/fifa96_probe.py`
- Create: `tests/test_probe.py`
- Modify: `CMakeLists.txt` (add `test_probe` in the Python block)

**Interfaces:**
- Frame wire format (frozen by this plan; the TSR in Task 2 emits it): `type:u8, seq:u16 LE, len:u16 LE, payload[len]`.
- `T_PROBE = 0x08`; payload = 16 bytes = four LE u32: `site`, `caller_lo`, `caller_hi`, `target_ret` (site id, caller return address low/high 16, runtime return address inside the target = `target_link + overwrite + delta`).
- `iter_frames(data: bytes)` → yields `(ftype, seq, payload, offset)`; raises `ValueError` on truncation; unknown types are yielded (callers filter).
- `probe_frames(data: bytes) -> list[dict]` → only `T_PROBE` frames with `len == 16`, each dict `{"offset", "site", "caller_lo", "caller_hi", "target_ret"}`.
- `normalize(frame: dict, target_link: int, overwrite: int) -> dict` → adds `{"caller_ret": lo | hi<<16, "delta": target_ret − (target_link + overwrite), "caller_link": caller_ret − delta, "target_ret_link": target_link + overwrite}`.
- CLI: `python3 tools/fifa96_probe.py TRACE [--target-link HEX] [--overwrite N] [--expect-site N]`; prints one line per probe frame; `--expect-site` exits 1 when absent.

- [ ] **Step 1: Write the failing tests**

```python
# tests/test_probe.py
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import fifa96_probe as probe  # noqa: E402


def frame(ftype, payload, seq=0):
    return bytes([ftype]) + struct.pack("<HH", seq, len(payload)) + payload


class TestFrames(unittest.TestCase):
    def test_iter_frames_roundtrip(self):
        stream = frame(0x02, b"A" * 18) + frame(0x08, b"B" * 16, seq=0)
        got = list(probe.iter_frames(stream))
        self.assertEqual([(t, s, len(p)) for t, s, p, _ in got],
                         [(0x02, 0, 18), (0x08, 0, 16)])

    def test_iter_frames_truncated_raises(self):
        with self.assertRaises(ValueError):
            list(probe.iter_frames(frame(0x08, b"B" * 16)[:-3]))

    def test_probe_frames_filters_type_and_length(self):
        payload = struct.pack("<IIII", 1, 0xDE32, 0x0037, 0x3B071F)
        stream = (frame(0x02, b"A" * 18)
                  + frame(0x08, payload)
                  + frame(0x08, b"short"))
        got = probe.probe_frames(stream)
        self.assertEqual(len(got), 1)
        self.assertEqual(got[0]["site"], 1)
        self.assertEqual(got[0]["caller_lo"], 0xDE32)
        self.assertEqual(got[0]["caller_hi"], 0x0037)
        self.assertEqual(got[0]["target_ret"], 0x3B071F)

    def test_normalize_derives_delta_and_link(self):
        f = {"site": 1, "caller_lo": 0xDE32, "caller_hi": 0x0037,
             "target_ret": 0x3B071F}
        n = probe.normalize(f, target_link=0x9E718, overwrite=7)
        self.assertEqual(n["caller_ret"], 0x37DE32)
        self.assertEqual(n["delta"], 0x2D1000)
        self.assertEqual(n["caller_link"], 0xCAE32)
        self.assertEqual(n["target_ret_link"], 0x9E71F)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python3 tests/test_probe.py`
Expected: FAIL/error `ModuleNotFoundError: No module named 'fifa96_probe'` (or import failure).

- [ ] **Step 3: Implement the decoder**

```python
# tools/fifa96_probe.py
#!/usr/bin/env python3
"""fifa96_probe.py — decode T_PROBE frames from a capture trace.

Frame wire format (emitted by tsr/fifa96_capture.asm):
    type:u8, seq:u16 LE, len:u16 LE, payload[len]
T_PROBE (0x08) payload = four LE u32: site, caller_lo, caller_hi, target_ret.

Runtime addresses are relocated; the delta is derived per run from
target_ret - (target_link + overwrite) and used to normalize the caller
return address back to link space.
"""
import argparse
import struct
import sys

T_PROBE = 0x08
PROBE_LEN = 16


def iter_frames(data):
    off = 0
    n = len(data)
    while off < n:
        if off + 5 > n:
            raise ValueError(f"truncated frame header at 0x{off:x}")
        ftype = data[off]
        seq, plen = struct.unpack_from("<HH", data, off + 1)
        off += 5
        if off + plen > n:
            raise ValueError(f"truncated payload at 0x{off:x}")
        yield ftype, seq, data[off:off + plen], off
        off += plen


def probe_frames(data):
    out = []
    for ftype, _seq, payload, off in iter_frames(data):
        if ftype != T_PROBE or len(payload) != PROBE_LEN:
            continue
        site, lo, hi, tgt = struct.unpack("<IIII", payload)
        out.append({"offset": off, "site": site, "caller_lo": lo,
                    "caller_hi": hi, "target_ret": tgt})
    return out


def normalize(frame, target_link, overwrite):
    n = dict(frame)
    n["caller_ret"] = frame["caller_lo"] | (frame["caller_hi"] << 16)
    n["delta"] = frame["target_ret"] - (target_link + overwrite)
    n["caller_link"] = n["caller_ret"] - n["delta"]
    n["target_ret_link"] = target_link + overwrite
    return n


def main(argv=None):
    ap = argparse.ArgumentParser(description="decode T_PROBE frames")
    ap.add_argument("trace")
    ap.add_argument("--target-link", type=lambda s: int(s, 0), default=None)
    ap.add_argument("--overwrite", type=int, default=None)
    ap.add_argument("--expect-site", type=lambda s: int(s, 0), default=None)
    args = ap.parse_args(argv)
    with open(args.trace, "rb") as fh:
        data = fh.read()
    frames = probe_frames(data)
    for f in frames:
        if args.target_link is not None and args.overwrite is not None:
            n = normalize(f, args.target_link, args.overwrite)
            print(f'T_PROBE site={f["site"]} caller=0x{n["caller_ret"]:08x} '
                  f'delta=0x{n["delta"]:x} caller_link=0x{n["caller_link"]:x} '
                  f'target_link=0x{n["target_ret_link"]:x}')
        else:
            print(f'T_PROBE site={f["site"]} caller_lo=0x{f["caller_lo"]:04x} '
                  f'caller_hi=0x{f["caller_hi"]:04x} '
                  f'target_ret=0x{f["target_ret"]:08x}')
    print(f"probe_frames={len(frames)}")
    if args.expect_site is not None:
        hit = any(f["site"] == args.expect_site for f in frames)
        print(f"expect_site=0x{args.expect_site:x} hit={hit}")
        return 0 if hit else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 4: Register the test and run the suite**

```cmake
# CMakeLists.txt, inside the if(Python3_Interpreter_FOUND) block
  add_test(NAME test_probe COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_probe.py)
```

Run: `make test`
Expected: 15/15 pass (`test_probe` new).

- [ ] **Step 5: Commit**

```bash
git add tools/fifa96_probe.py tests/test_probe.py CMakeLists.txt
git commit -m "feat(trace): decode DPMI probe frames and normalize caller addresses"
```

---

### Task 2: TSR INT 61 probe handler + real-mode smoke

**Files:**
- Modify: `tsr/fifa96_capture.asm`
- Create: `tsr/probe_stub.asm`
- Create: `tools/int61_smoke.sh`

**Interfaces:**
- Consumes: `T_PROBE = 0x08` frame layout from Task 1; `tools/fifa96_probe.py --expect-site`.
- Produces: resident `INT 61h` handler that emits one `T_PROBE` frame per invocation with payload `site, caller_lo, caller_hi, target_ret` read from `ESI, EBX, EDX, EBP` (the registers the DPMI structure delivers). Entry `int61` label; `old_int61 dd 0`; `probe_buf times 16 db 0`.

- [ ] **Step 1: Add the frame type and resident buffer**

In `tsr/fifa96_capture.asm`, next to the existing type constants:

```asm
T_PROBE   equ 0x08
```

In the resident data block, next to `old_int1`:

```asm
old_int61  dd 0
probe_buf  times 16 db 0           ; ESI,EBX,EDX,EBP dwords from DPMI
```

- [ ] **Step 2: Add the handler**

Append after the `int1` handler, before `tsr_end:` (offsets follow `pushad` order: EDI@+0, ESI@+4, EBP@+8, ESP@+12, EBX@+16, EDX@+20, ECX@+24, EAX@+28):

```asm
; ─────────── INT-61 DPMI probe dump (T_PROBE) ───────────
; Entered via DPMI INT 31h AX=0300h (simulated real-mode interrupt) from the
; PM cave. The DPMI host loads ESI/EBX/EDX/EBP from the caller's register
; structure, so the 16-bit handler reads the full 32-bit values with 32-bit
; operand sizes (values are constructed 16-bit-safe: site <= 0xFFFF, the
; caller address is split lo/hi, and target_ret fits under 0x100000000).
; INT 61h is dedicated to this probe; the saved vector is not chained.
int61:
        pushad
        mov  bp, sp
        push ds
        push cs
        pop  ds
        mov  eax, [bp+4]                ; ESI = site id
        mov  [probe_buf], eax
        mov  eax, [bp+16]               ; EBX = caller return address, low 16
        mov  [probe_buf+4], eax
        mov  eax, [bp+20]               ; EDX = caller return address, high 16
        mov  [probe_buf+8], eax
        mov  eax, [bp+8]                ; EBP = target+overwrite (runtime)
        mov  [probe_buf+12], eax
        mov  al, T_PROBE
        mov  si, probe_buf
        mov  cx, 16
        call send_frame
        pop  ds
        popad
        iret
```

- [ ] **Step 3: Install the vector**

In `install:`, beside the `0x3560`/`0x2560` pair:

```asm
        mov  ax, 0x3561
        int  0x21
        mov  [cs:old_int61], bx
        mov  [cs:old_int61+2], es

        mov  ax, 0x2561
        mov  dx, int61
        int  0x21
```

- [ ] **Step 4: Write the real-mode smoke stub**

```asm
; tsr/probe_stub.asm — INT 61h smoke: sets four sentinel values and exits.
; nasm -f bin tsr/probe_stub.asm -o STUB.COM
        org 0x100
        bits 16
        mov  esi, 0x0000BEEF
        mov  ebx, 0x00001234
        mov  edx, 0x00005678
        mov  ebp, 0x00009ABC
        int  0x61
        mov  ax, 0x4C00
        int  0x21
```

- [ ] **Step 5: Write the headless smoke script**

```sh
#!/bin/sh
# tools/int61_smoke.sh — TSR + real-mode INT 61h stub -> serial -> decoder.
set -eu
DIR=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d -t fifa96int61.XXXXXX)
trap 'rm -rf "$WORK"' EXIT
[ -f "$DIR/build/FIFACAP.COM" ] || { echo "missing build/FIFACAP.COM (make tsr)" >&2; exit 1; }
nasm -f bin "$DIR/tsr/probe_stub.asm" -o "$WORK/STUB.COM"
cp "$DIR/build/FIFACAP.COM" "$WORK/"
cat > "$WORK/dosbox.conf" <<EOF
[dosbox]
machine=svga_s3
memsize=16

[serial]
serial1=file file:$WORK/trace.bin multiplier:100

[sdl]
fullscreen=false

[autoexec]
@echo off
MOUNT C "$WORK"
C:
FIFACAP.COM
STUB.COM
exit
EOF
timeout 60 dosbox-x -conf "$WORK/dosbox.conf" -fastlaunch -nopromptfolder -silent >/dev/null 2>&1 || true
echo "--- frames ---"
python3 "$DIR/tools/fifa96_probe.py" "$WORK/trace.bin" --expect-site 0xBEEF
```

- [ ] **Step 6: Build and run the smoke**

Run: `make tsr && sh tools/int61_smoke.sh`
Expected: `FIFACAP.COM: <size>` + sha256; then `T_PROBE site=48879 caller_lo=0x1234 caller_hi=0x5678 target_ret=0x9abc`, `probe_frames=1`, `expect_site=0xbeef hit=True`, exit 0.

- [ ] **Step 7: Run the suite and commit**

Run: `make test` (15/15 unchanged).

```bash
git add tsr/fifa96_capture.asm tsr/probe_stub.asm tools/int61_smoke.sh
git commit -m "feat(tsr): add INT 61 DPMI probe dump handler and real-mode smoke"
```

---

### Task 3: Trampoline injector and capture-script options

**Files:**
- Create: `tools/fifa96_patch.py`
- Create: `tests/test_patch.py`
- Modify: `CMakeLists.txt` (add `test_patch`)
- Modify: `run-fifa96-capture.sh`

**Interfaces:**
- Consumes: `tools/fifa96_le.parse()` (objects with `base`, `pages`, `page_index`; `data_offset`, `page_size`, `last_page`) and the cave template below.
- Produces: `patch_iso(data: bytes, target: int, cave: int, site_id: int) -> bytes` (new ISO image with the trampoline applied to `FIFA96.EXE` in place, same total size); `build_cave(target, overwrite, site_id, cave_link) -> bytes`; `overwrite_len(image_bytes, target) -> int` (instruction prefix ≥5 via ndisasm); helper `find_iso_file(data, name) -> (lba, size)`.
- Cave contract (must match Task 2's handler): sets `ESI=site`, `EBX/EDX=caller ret lo/hi`, `EBP=target+overwrite runtime`, `AX=0x0300`, `BL=0x61`, `CX=0`, `ES:EDI` = 48-byte zeroed DPMI register structure on the stack at slots `+0x04 ESI`, `+0x08 EBP`, `+0x10 EBX`, `+0x14 EDX`.

- [ ] **Step 1: Write the failing tests**

```python
# tests/test_patch.py
import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fifa96_patch as patch  # noqa: E402
import fifa96_le as le  # noqa: E402

ISO = ROOT / "game" / "FIFAPCCD96.iso"
TARGET = 0x9E718
CAVE = 0x6728D


class TestIso(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.iso = ISO.read_bytes()

    def test_find_fifa96_exe(self):
        lba, size = patch.find_iso_file(self.iso, "FIFA96.EXE")
        self.assertGreater(lba, 16)
        self.assertGreater(size, 0x180000)
        exe = self.iso[lba * 2048:lba * 2048 + size]
        self.assertEqual(exe[:2], b"MZ")
        info = le.parse(exe)
        self.assertEqual(info["header_offset"], 0x290A4)

    def test_patch_changes_only_target_and_cave(self):
        out = patch.patch_iso(self.iso, TARGET, CAVE, 1)
        self.assertEqual(len(out), len(self.iso))
        diffs = [i for i in range(len(out)) if out[i] != self.iso[i]]
        self.assertGreater(len(diffs), 5)
        # every changed byte lies in one of the two patched regions
        lba, size = patch.find_iso_file(self.iso, "FIFA96.EXE")
        base = lba * 2048
        exe = bytearray(self.iso[base:base + size])
        off_t = patch.link_to_file_offset(le.parse(bytes(exe)), TARGET)
        off_c = patch.link_to_file_offset(le.parse(bytes(exe)), CAVE)
        for d in diffs:
            rel = d - base
            self.assertTrue(off_t <= rel < off_t + 8 or
                            off_c <= rel < off_c + 0x103,
                            f"unexpected diff at file offset 0x{rel:x}")


class TestCave(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.iso = ISO.read_bytes()
        lba, size = patch.find_iso_file(cls.iso, "FIFA96.EXE")
        cls.info = le.parse(cls.iso[lba * 2048:lba * 2048 + size])

    def test_overwrite_len_prefix(self):
        self.assertEqual(patch.overwrite_len(self.info["image"], TARGET), 7)
        self.assertEqual(patch.overwrite_len(self.info["image"], 0x9E860), 6)
        self.assertEqual(patch.overwrite_len(self.info["image"], 0xC9D10), 6)
        self.assertEqual(patch.overwrite_len(self.info["image"], 0xCAD30), 6)

    def test_cave_contains_displaced_bytes_and_jump(self):
        ow = patch.overwrite_len(self.info["image"], TARGET)
        cave = patch.build_cave(TARGET, ow, 1, CAVE)
        self.assertLess(len(cave), 0x103)
        displaced = self.info["image"][TARGET:TARGET + ow]
        tgt = TARGET + ow
        jmp_off = cave.rindex(b"\xe9")
        rel = struct.unpack_from("<i", cave, jmp_off + 1)[0]
        self.assertEqual(CAVE + jmp_off + 5 + rel, tgt)
        self.assertIn(displaced, cave)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python3 tests/test_patch.py`
Expected: `ModuleNotFoundError: No module named 'fifa96_patch'`.

- [ ] **Step 3: Implement the injector**

Required pieces (full implementation; nasm/ndisasm via `subprocess`):

```python
#!/usr/bin/env python3
"""fifa96_patch.py — inject PM trace trampolines into a FIFA96.EXE ISO copy.

The cave (32-bit, position-independent apart from its org) reports the caller
return address through DPMI INT 31h AX=0300h to the TSR's INT 61h handler.
Everything is patched in a COPY of the ISO; the input is never written.
"""
import argparse
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fifa96_le as le  # noqa: E402

SECTOR = 2048
CAVE_TEMPLATE = r"""
bits 32
org {cave:#x}
cave:
        pushad
        pushfd
        push es
        mov  ax, ss
        mov  es, ax
        mov  ebx, [esp+44]          ; caller return address (relocated)
        mov  ebp, [esp+40]          ; return into target (relocated)
        mov  edx, ebx
        shr  edx, 16
        and  ebx, 0xFFFF
        mov  esi, {site:#x}
        sub  esp, 48
        mov  edi, esp
        xor  eax, eax
        mov  ecx, 12
        cld
        rep  stosd
        mov  [esp+0x04], esi        ; ESI slot
        mov  [esp+0x08], ebp        ; EBP slot
        mov  [esp+0x10], ebx        ; EBX slot
        mov  [esp+0x14], edx        ; EDX slot
        mov  ax, 0x0300
        mov  bx, 0x0061
        xor  cx, cx
        int  0x31
        add  esp, 48
        pop  es
        popfd
        popad
{displaced}
        jmp  {resume:#x}
"""
```

- `overwrite_len(image, target)`: run `ndisasm -b32 -o <target> -` on `image[target:target+32]` (stdin bytes, `text=False`), parse `^[0-9A-F]+ +([0-9A-F]+)`, accumulate instruction byte lengths until `>= 5`, return the sum (cap 16, raise if a branch opcode `e8/e9/eb/7x/0f8x` appears in the prefix).
- `link_to_file_offset(info, link)`: find the object with `base <= link < base + vsize`; `page = (link - base) // page_size`; reject `page >= obj["pages"]` (BSS); `off = info["data_offset"]`; add full page sizes for all stored pages before this object's pages; within the object, add `page_size * page` (the global last page is short: if the global page index == `info["pages"] - 1`, use `info["last_page"]`); return `off + (link - base) % page_size`.
- `find_iso_file(data, name)`: PVD at sector 16 (`0x8000`, check `\x01CD001`); root directory record at PVD+156 (extent LE u32 at +2, size at +10); walk 2048-byte directory sectors record by record (`len` byte at +0; skip 0 → next sector); match `name` case-insensitively with optional `;1`; return `(extent, size)`.
- `build_cave(target, overwrite, site_id, cave_link)`: format the template with `displaced` = `"        db " + ",".join(f"0x{b:02x}" for b in image[target:target+overwrite])` and `resume = target + overwrite`; assemble with `nasm -f bin` in a temp dir; return the bytes; assert `len <= 0x103`.
- `patch_iso(data, target, cave, site_id)`: `lba, size = find_iso_file(...)`; `exe = data[lba*SECTOR:lba*SECTOR+size]`; `info = le.parse(exe)`; `ow = overwrite_len(info["image"], target)`; `cave_bytes = build_cave(target, ow, site_id, cave)`; `out = bytearray(data)`; write cave bytes at `lba*SECTOR + link_to_file_offset(info, cave)`; write `b"\xe8" + rel32(cave, target+5)` at `lba*SECTOR + link_to_file_offset(info, target)`; return `bytes(out)`.
- CLI: `--iso IN --out OUT --target HEX --cave HEX --site-id N`; print target overwrite length, cave length, and the patched file-size delta (must be 0).

- [ ] **Step 4: Run the injector tests**

Run: `python3 tests/test_patch.py`
Expected: PASS (4 tests).

- [ ] **Step 5: Add capture-script options (default behavior unchanged)**

In `run-fifa96-capture.sh`:

```sh
ISO=${ISO:-"$DIR/game/FIFAPCCD96.iso"}     # replaces the fixed assignment
HEADLESS=${HEADLESS:-0}
TIMEOUT=${TIMEOUT:-0}
```

and replace the final invocation with:

```sh
if [ "$HEADLESS" = "1" ]; then
  EXTRA="-silent"
else
  EXTRA=""
fi
if [ "$TIMEOUT" -gt 0 ]; then
  timeout -s TERM "$TIMEOUT" dosbox-x -conf "$CONF" -fastlaunch -nopromptfolder $EXTRA || true
else
  dosbox-x -conf "$CONF" -fastlaunch -nopromptfolder $EXTRA
fi
```

Also add `[ -f "$ISO" ]` check against the overridden path (replace the existing `[ -f "$ISO" ]` check's message unchanged). Verify `sh -n run-fifa96-capture.sh`.

- [ ] **Step 6: Register the test, run the suite, commit**

```cmake
  add_test(NAME test_patch COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_patch.py)
```

Run: `make test`
Expected: 16/16 pass.

```bash
git add tools/fifa96_patch.py tests/test_patch.py CMakeLists.txt run-fifa96-capture.sh
git commit -m "feat(patch): inject PM trace trampolines into ISO copies"
```

---

### Task 4: End-to-end DPMI validation on the decompressor

**Files:**
- Create: `tools/trace_probe.sh`
- Create: `docs/ghidra/FU11_dpmi_probe_findings.md`

**Interfaces:**
- Consumes: `tools/fifa96_patch.py` CLI, `tools/fifa96_probe.py` CLI, `run-fifa96-capture.sh` env (`ISO`, `SESSION`, `HEADLESS`, `TIMEOUT`), TSR `INT 61h`.
- Produces: `tools/trace_probe.sh TARGET_LINK SITE_ID SESSION` → patched ISO under `build/`, capture under `captures/session-<SESSION>/trace.bin`, decoded probe lines on stdout; FU-11 doc with the mechanism and the first target's verified caller set.

- [ ] **Step 1: Write the one-command runner**

```sh
#!/bin/sh
# tools/trace_probe.sh TARGET_LINK SITE_ID SESSION
# Patches a copy of the ISO with a trampoline at TARGET_LINK, runs the game
# headless under the capture TSR, and decodes T_PROBE frames.
set -eu
DIR=$(cd "$(dirname "$0")/.." && pwd)
TARGET=$1
SITE=$2
SESSION=$3
ISO="$DIR/build/fifa96-trace-$SITE.iso"
python3 "$DIR/tools/fifa96_patch.py" --iso "$DIR/game/FIFAPCCD96.iso" \
    --out "$ISO" --target "$TARGET" --cave 0x6728D --site-id "$SITE"
ISO="$ISO" SESSION="$SESSION" HEADLESS=1 TIMEOUT=120 \
    "$DIR/run-fifa96-capture.sh"
python3 "$DIR/tools/fifa96_probe.py" \
    "$DIR/captures/session-$SESSION/trace.bin" \
    --target-link "$TARGET" --expect-site "$SITE"
```

- [ ] **Step 2: Validate end-to-end on FUN_0009e718**

Run: `make tsr && sh tools/trace_probe.sh 0x9E718 1 probe-9e718`
Expected: capture completes; decoder prints at least one line with `site=1`, a constant `delta` across frames, `caller_link` equal to one of the static return addresses `{0x9E86C, 0x9E884, 0xCAE32}` (FU-8/FU-9 direct-call census), and `target_link=0x9e71f`.

If zero frames: do NOT fabricate. Re-run once with `TIMEOUT=180`; if still zero, report BLOCKED with the decoder output, the trace size, and `grep -c` of any `T_PROBE` bytes — the controller rules on the DPMI-unsupported fallback.

- [ ] **Step 3: Document the mechanism and the first result**

Create `docs/ghidra/FU11_dpmi_probe_findings.md` with: the cave/handler contract, the measured delta for the run, the verbatim decoded probe lines, and the cross-check row (`caller_link` ↔ static call site from the FU-8/FU-9 census), plus the session provenance line (`captures/session-probe-9e718/trace.bin`, size in bytes). State explicitly that the delta is measured, not assumed.

- [ ] **Step 4: Run the suite and commit**

Run: `make test` (16/16).

```bash
git add tools/trace_probe.sh docs/ghidra/FU11_dpmi_probe_findings.md
git commit -m "feat(trace): end-to-end DPMI probe on the decompressor entry"
```

---

### Task 5: Batch the remaining entry points

**Files:**
- Modify: `docs/ghidra/FU11_dpmi_probe_findings.md`

**Interfaces:**
- Consumes: Task 4's validated `tools/trace_probe.sh` and decoder; static caller censuses from `docs/ghidra/FU8_loader_chain.md` / `FU9_format_consumers.md`.
- Produces: one capture + decoder table per target in FU-11, each with the measured delta, matched static call site, and unmatched findings.

- [ ] **Step 1: Run the three remaining targets**

Run (sequentially; ~2 min each):

```sh
sh tools/trace_probe.sh 0x9E860 2 probe-9e860
sh tools/trace_probe.sh 0xC9D10 3 probe-c9d10
sh tools/trace_probe.sh 0xCAD30 4 probe-cad30
```

Expected static cross-checks: `0x9E860` return addresses in `{0x14C65, 0x14D76, 0x18CE8, 0x23C0C, 0x24DE6, 0x26E47, 0x4A32A, 0x4A376, 0x4A42B, 0x4A5DC, 0x4B000, 0x78E5E}`; `0xC9D10` in `{0xC571A, 0xC5742}`; `0xCAD30` in `{0xC9D75, 0xC9DC2, 0xCACB4, 0xCACD0, 0xCACEF, 0xCAD0C, 0xCAD24}`. Any address outside its list is recorded as an unmatched runtime caller (potential runtime-built dispatch) — not discarded.

- [ ] **Step 2: Append the tables**

For each target append: overwrite length, measured delta, verbatim decoder lines, matched/unmatched verdicts, and the capture provenance. Keep each target's table self-contained.

- [ ] **Step 3: Run the suite and commit**

Run: `make test` (16/16).

```bash
git add docs/ghidra/FU11_dpmi_probe_findings.md
git commit -m "docs(fu11): record runtime callers for wrapper and loader entries"
```

---

## Self-Review (ran before save)

- **Spec coverage:** PM trampoline at `0x6728D` (T3), DPMI `INT 31h AX=0300h` with obj4-free stack structure (T3 cave), TSR `INT 60h`→`INT 61h` change per FU-10's intent with the reason recorded (T2; `INT 60` is owned by the existing real-mode `CD 60` probe, whose site match would miss the DPMI frame — dedicated vector avoids corrupting single-step re-arm), targets `FUN_0009e718/FUN_0009e860/FUN_000c9d10/FUN_000cad30` (T4/T5), verification vs static caller lists (T4/T5), linear delta measured not assumed (constraints + T4), headless-safe (T4/T5). FU-10's "risks/fallbacks" are honored: DPMI-unsupported path is an explicit BLOCKED-with-evidence gate (T4 Step 2).
- **Placeholder scan:** no TBD/TODO; tests and asm are verbatim; the injector's five functions are specified with their exact algorithms and signatures; every sample value (return addresses, overwrite lengths, site ids, expected payload `0xBEEF/0x1234/0x5678/0x9ABC`) is concrete.
- **Type consistency:** `T_PROBE=0x08` and the 16-byte payload order `site, caller_lo, caller_hi, target_ret` are identical in T1 (decoder), T2 (asm), T3 (cave slot mapping ESI/EBX/EDX/EBP), T4/T5 (decoder CLI). `overwrite_len` expected values (7/6/6/6) match the disassembled entry prefixes. `patch_iso`'s diff test bounds use the same `link_to_file_offset` the tool uses.

(End of file)
