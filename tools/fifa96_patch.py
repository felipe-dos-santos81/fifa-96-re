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
CAVE_CAPACITY = 0x103
DEFAULT_ISO = Path(__file__).resolve().parents[1] / "game" / "FIFAPCCD96.iso"
CAVE_TEMPLATE = r"""
bits 32
org {cave:#x}
cave:
        pushad
        pushfd
        push es
{es_setup}
        mov  ebx, [esp+44]          ; caller return address (relocated)
        mov  ebp, [esp+40]          ; call's return (target+5, relocated)
        add  ebp, {adjust}          ; resume is target+overwrite
        mov  edx, ebx
        shr  edx, 16
        and  ebx, 0xFFFF
{set_site}
        sub  esp, 48
        mov  edi, esp
        xor  eax, eax
        mov  ecx, 12
        cld
        rep  stosd
        mov  edi, esp               ; ES:EDI -> structure base for INT 31h
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
        add  esp, 4                 ; discard the call's return address
{displaced}
        jmp  {resume:#x}
"""

# --- VGT golden-record capture (FU-20/FU-21) -------------------------------
# Two trampolines share the single obj1 zero run (link 0x6728D, 0x103 bytes):
#   [scratch:u32=0][entry probe][return capture]
#
# FU-21: the return hook keeps one golden slot per observed method byte
# instead of a single first-record slot. The scratch cell is the head of a
# singly linked list of heap blocks (0 = nothing captured yet); each block is
# self-describing, so extraction walks the chain and copy-once falls out of
# checking the method field during the walk. The chain costs ~15 bytes over
# the single-slot FU-20 template, paid for by the entry probe dropping its
# caller split (it now carries the observed method byte in the caller_lo
# field). The allocator FUN_00098bf8 preserves only ESI/EDI (it clobbers
# EAX/EBX/ECX/EDX; live session vgt21-1 caught EBX returning as 0), so the
# decoded length is kept on the stack across the call and the method byte in
# EDI.
#
# Block layout (heap, 0x10 + in_cap + out_cap bytes):
#   +0x00 next pointer (guest runtime address; 0 = tail)
#   +0x04 method id (raw stream[0], zero-extended dword)
#   +0x08 decoded length (out_len)
#   +0x0C magic 'FVGT' (0x54475646)
#   +0x10 bounded input slice (in_cap)
#   +0x10+in_cap exact decoded output (out_len)
#
# FU-20's single block (magic at +0, no next) is still understood by the
# extraction tool so the committed record-10 dump remains re-extractable.
#
# Entry probe: the proven FU-11 DPMI descriptor, but the caller-address split
# is dropped (extraction only needs site + the raw call return for the load
# delta) and the observed method byte (stream[0]) is reported in the
# caller_lo field, making the trace a per-method liveness record. EBX/EDX
# slots stay zero from the structure zero-fill (no caller attribution).
#
# Return capture: stream and dst are read from the caller's argument slots;
# the decoded length is the ESI saved by the target's own PUSH ESI. A 0xFB
# signature plus a nontrivial/<=out_cap length filter selects qualifying
# records; a list walk skips methods that already own a slot. The golden
# block is allocated from the game's own heap via FUN_00098bf8 (cdecl:
# debug-tag string, size, flags) and read back out of a host RAM dump. The
# tag argument is the scratch cell: on the first capture it is zero (empty
# string); later it holds a small heap pointer whose top byte is zero, so the
# allocator's basename helper stops inside the pointer bytes. This avoids the
# separate low-address relocation base (0x2D1000 in the FU-20 runs) that
# &DAT_000033cc / &DAT_0000420c carry; that base is not the client image
# delta (0x1FC000) and is not otherwise derivable from the code delta.
VGT_ENTRY_TEMPLATE = r"""
bits 32
org {cave:#x}
vgt_entry_probe:
        pushad
        pushfd
        push es
        mov  ax, ss
        mov  es, ax
        mov  ebp, [esp+40]              ; call's return (target+5, runtime)
        add  ebp, {adjust}              ; resume is target+overwrite
        mov  ebx, [esp+48]              ; stream (caller arg slot; the
                                        ; patched call adds a return address,
                                        ; so [esp+44] is the caller's return)
        movzx ebx, byte [ebx]           ; observed method byte
        mov  esi, {site:#x}
        sub  esp, 48
        mov  edi, esp
        xor  eax, eax
        mov  ecx, 12
        cld
        rep  stosd
        mov  edi, esp                   ; ES:EDI -> real-mode register struct
        mov  [esp+0x04], esi            ; ESI slot = site id
        mov  [esp+0x08], ebp            ; EBP slot = target resume (runtime)
        mov  [esp+0x10], ebx            ; EBX slot = observed method byte
        mov  ax, 0x0300
        mov  bx, 0x0061
        xor  cx, cx
        int  0x31
        add  esp, 48
        pop  es
        popfd
        popad
        add  esp, 4                     ; discard the call's return address
{displaced}
        jmp  {resume:#x}
"""

VGT_RETURN_TEMPLATE = r"""
bits 32
org {cave:#x}
vgt_capture_return:
        pushad
        pushfd
        push es
        mov  esi, [esp+0x28]            ; call pushed target_ret+5 (runtime)
        sub  esi, {call_ret:#x}         ; ESI = load delta (runtime-link)
        mov  ebp, [esp+0x3c]            ; stream (caller arg slot)
        test ebp, ebp
        jz   .done
        cmp  byte [ebp+1], 0xfb         ; compression signature byte
        jne  .done
        mov  ecx, [esp+0x0c]            ; pushad-saved ESI = decoded length
        lea  edx, [ecx-{min_out:#x}]
        cmp  edx, {range_cap:#x}
        ja   .done                      ; keep min_out <= len <= out_cap
        movzx edi, byte [ebp]           ; observed method (callee-saved)
        mov  edx, [esi+{scratch:#x}]    ; head of the captured-slot list
.scan:
        test edx, edx
        jz   .capture
        cmp  dword [edx+4], edi         ; method already has a slot?
        je   .done
        mov  edx, [edx]
        jmp  .scan
.capture:
        push ecx                        ; out_len survives the allocator
        push 0
        push {alloc_size:#x}
        lea  eax, [esi+{scratch:#x}]    ; cell = debug tag string
        push eax
        call {alloc:#x}                 ; FUN_00098bf8(tag, size, 0)
        add  esp, 0xc
        pop  ecx                        ; out_len
        test eax, eax
        jz   .done
        mov  edx, [esi+{scratch:#x}]    ; old head
        mov  dword [eax], edx           ; block->next
        mov  dword [esi+{scratch:#x}], eax
        mov  [eax+8], ecx               ; out_len
        mov  [eax+4], edi               ; method id
        mov  dword [eax+0xc], {magic:#x}
        mov  ebx, eax
        mov  eax, [esp+0x40]            ; dst (caller arg slot)
        push ds
        pop  es
        mov  esi, ebp
        lea  edi, [ebx+0x10]            ; input slice at golden+0x10
        push ecx                        ; out_len across the input copy
        mov  ecx, {in_cap:#x}
        cld
        rep  movsb
        pop  ecx
        mov  esi, eax
        rep  movsb                      ; output follows the input slice
.done:
        pop  es
        popfd
        popad
        add  esp, 4                     ; discard the call's return address
{displaced}
        jmp  {resume:#x}
"""

VGT_MAGIC = 0x54475646                  # 'FVGT'

# --- fVGT frame capture (FU-31) --------------------------------------------
# A single return-side trampoline patched over the vgt_decode_f epilogue at
# 0xAE20B (cdecl `(ctx, chunk)`). No entry probe: the caller frame is still
# live at the return, so ctx is [esp+0x28+0x3c] and chunk [esp+0x28+0x40]
# after the cave's pushad/pushfd/push es ([esp+0x28] is the patched call's
# return address, 0xAE210 + load delta). Verified whole-instruction overwrite
# at 0xAE20B is 7 bytes -- `MOV EAX,[ESP+0x14]` is FOUR bytes
# (8B 44 24 14), not three as the FU-31 brief assumed -- so the displaced
# bytes are 8B 44 24 14 83 C4 28 and the resume is 0xAE212 (not 6/0xAE211).
#
# A qualifying call has chunk[0..3] == 'fVGT' and a nonzero canvas dims
# product from ctx[0] (pitch) * ctx[1] (height) within out_cap. The decoded
# canvas is the surface vgt_decode_f composited into: ctx[10] (DECIMAL
# index 10 = byte 0x28) after its entry swap, pixels at +0x10
# (vgt_dispatch returns ctx[10]). NOTE: the FU-31 brief's "canvas ptr
# ctx[0xc], byte 0x30" is the width*4 index array; byte 0x40 (index 0x10)
# is the row table. Copying either produced table/noise data in sessions
# fvgt-1/fvgt-2/fvgt-4.
# Capture-latest (errata for the brief's copy-once): one block is allocated
# on the first qualifying call and every later qualifying call overwrites
# its input/output in place (a lost magic/method re-allocates). Copy-once
# captured the first qualifying chunk, whose canvas is an early mostly
# unpainted frame (session fvgt-2); capture-latest leaves the converged
# intro frame for the 300s dump to read. One block per run either way.
# Block layout matches FU-20/21:
#   +0x00 next=0, +0x04 method=0x66, +0x08 out_len, +0x0C 'FVGT' magic,
#   +0x10 in_cap input slice, +0x10+in_cap out_len output bytes.
FVGT_TAG = 0x54475666                    # 'fVGT' chunk tag (LE dword)
FVGT_METHOD = 0x66                       # golden block method id
# Measured max in-memory fVGT chunk over the ISO's TGV assets: 0x3AB8
# (VID_SCB0); the FU-31 brief's 0x3000 cap truncated the live captured
# frame (session fvgt-3, first converged chunk len 0x3774).
FVGT_IN_CAP = 0x4000
FVGT_OUT_CAP = 0x40000                   # 320x240 = 0x12C00, 2x margin

FVGT_RETURN_TEMPLATE = r"""
bits 32
org {cave:#x}
fvgt_capture_return:
        pushad
        pushfd
        push es
        mov  esi, [esp+0x28]            ; call's return (0xAE210 + load delta)
        sub  esi, {call_ret:#x}         ; ESI = load delta
        mov  ebp, [esp+0x28+0x40]       ; chunk (caller arg slot)
        mov  ebx, [esp+0x28+0x3c]       ; ctx (caller arg slot)
        test ebp, ebp
        jz   .done
        test ebx, ebx
        jz   .done
        cmp  dword [ebp], {tag:#x}      ; 'fVGT'
        jne  .done
        movzx edx, word [ebp+8]         ; chunk field +8 (index count)
        test edx, edx
        jz   .done
        movzx eax, word [ebp+0xa]       ; chunk field +10 (raw blocks)
        test eax, eax
        jz   .done
        movzx ecx, word [ebx]           ; canvas pitch
        movzx edx, word [ebx+4]         ; canvas height
        test ecx, ecx
        jz   .done
        test edx, edx
        jz   .done
        imul ecx, edx                   ; out_len = pitch*height
        cmp  ecx, {out_cap:#x}
        ja   .done
        mov  edi, [ebx+0x28]            ; vgt_decode_f dest surface ctx[10]
        test edi, edi                   ; (index 10, byte 0x28; entry swap)
        jz   .done
        add  edi, 0x10                  ; pixel plane
        mov  eax, [esi+{scratch:#x}]    ; existing block (capture-latest)
        test eax, eax
        jz   .alloc
        cmp  dword [eax+0xc], {magic:#x}
        jne  .alloc                     ; lost magic: heap reset, re-alloc
        cmp  dword [eax+4], {method:#x}
        jne  .alloc
        jmp  .fill
.alloc:
        push ecx                        ; out_len survives the allocator
        push 0
        push {alloc_size:#x}
        lea  eax, [esi+{scratch:#x}]    ; cell = debug tag string
        push eax
        call {alloc:#x}                 ; FUN_00098bf8(tag, size, 0)
        add  esp, 0xc
        pop  ecx                        ; out_len
        test eax, eax
        jz   .done
        mov  edx, [esi+{scratch:#x}]    ; old head (0)
        mov  [eax], edx                 ; block->next
        mov  [esi+{scratch:#x}], eax
        mov  dword [eax+4], {method:#x} ; fixed fVGT method id
        mov  dword [eax+0xc], {magic:#x}
.fill:
        mov  [eax+8], ecx               ; out_len
        mov  edx, edi                   ; canvas (callee-saved EDI)
        mov  esi, ebp                   ; chunk
        lea  edi, [eax+0x10]            ; input slice at block+0x10
        push ecx                        ; out_len across the input copy
        mov  ecx, {in_cap:#x}
        push ds
        pop  es
        cld
        rep  movsb
        pop  ecx
        mov  esi, edx
        rep  movsb                      ; output follows the input slice
.done:
        pop  es
        popfd
        popad
        add  esp, 4                     ; discard the call's return address
{displaced}
        jmp  {resume:#x}
"""

# --- fVGT paired pre/post capture (FU-32) ----------------------------------
# The FU-31 single-frame return cave captures the composited post-state
# surface (ctx[10]+0x10 at the epilogue). vgt_decode_f's entry swap
# (ctx[10] <-> ctx[0xb]) leaves the pre-call surface live at ctx[0xb] at
# the return, and the composite call only writes ctx[10]+0x10, so the
# pre-state could be read at the return too. This rig instead follows the
# intended two-point design: a compact entry hook records the pre-call
# surface pointer (ctx[10] before the swap) in a static marker cell, and
# the return hook copies the chunk slice, the post surface (ctx[10]+0x10)
# and the pre surface (marker+0x10) into one extended block.
#
# Cave budget: the only spare executable obj1 zero run besides the FU-31
# run is 0x18A01 (0x2F bytes). A full snapshot entry cave (alloc + 76800
# B copy) does not fit and is unnecessary because the pre buffer survives
# the call; the 40-byte marker cave + 4-byte cell do.
FVGT_ENTRY_TEMPLATE = r"""
bits 32
org {cave:#x}
fvgt_entry_mark:
        push esi
        mov  esi, [esp+4]               ; call's return (0xADF01 + delta)
        lea  esi, [esi-{call_ret:#x}]   ; ESI = load delta
        mov  eax, [esp+0xc]             ; ctx (cdecl arg 1)
        mov  eax, [eax+0x28]            ; pre surface ctx[10] before swap
        mov  [esi+{marker:#x}], eax     ; hand off to the return cave
        pop  esi
        add  esp, 4                     ; discard the call's return address
{displaced}
        jmp  {resume:#x}
"""

FVGT_PAIR_TEMPLATE = r"""
bits 32
org {cave:#x}
fvgt_pair_return:
        pushad
        pushfd
        push es
        mov  esi, [esp+0x28]            ; call's return (0xAE210 + delta)
        sub  esi, {call_ret:#x}         ; ESI = load delta
        mov  ebp, [esp+0x28+0x40]       ; chunk (caller arg slot)
        mov  ebx, [esp+0x28+0x3c]       ; ctx (caller arg slot)
        test ebp, ebp
        jz   .done
        test ebx, ebx
        jz   .done
        cmp  dword [ebp], {tag:#x}      ; 'fVGT'
        jne  .done
        movzx edx, word [ebp+8]         ; chunk field +8 (index count)
        test edx, edx
        jz   .done
        movzx eax, word [ebp+0xa]       ; chunk field +10 (raw blocks)
        test eax, eax
        jz   .done
        movzx ecx, word [ebx]           ; canvas pitch
        movzx edx, word [ebx+4]         ; canvas height
        test ecx, ecx
        jz   .done
        test edx, edx
        jz   .done
        imul ecx, edx                   ; out_len = pitch*height
        cmp  ecx, {out_cap:#x}
        ja   .done
        mov  edi, [ebx+0x28]            ; post surface ctx[10] (post-swap)
        test edi, edi
        jz   .done
        mov  ebx, [esi+{marker:#x}]     ; pre surface = entry ctx[10]
        test ebx, ebx                   ; (ctx[0xb] after the entry swap)
        jz   .done
        add  edi, 0x10                  ; post pixel plane
        add  ebx, 0x10                  ; pre pixel plane
        mov  eax, [esi+{scratch:#x}]    ; existing block (capture-latest)
        test eax, eax
        jz   .alloc
        cmp  dword [eax+0xc], {magic:#x}
        jne  .alloc                     ; lost magic: heap reset, re-alloc
        jmp  .fill
.alloc:
        push ebx                        ; pre surface (allocator clobbers EBX)
        push ecx                        ; out_len survives the allocator
        push 0
        push {alloc_size:#x}
        lea  eax, [esi+{scratch:#x}]    ; cell = debug tag string
        push eax
        call {alloc:#x}                 ; FUN_00098bf8(tag, size, 0)
        add  esp, 0xc
        pop  ecx                        ; out_len
        pop  ebx                        ; pre surface
        test eax, eax
        jz   .done
        mov  [esi+{scratch:#x}], eax
        mov  byte [eax+4], {method:#x}  ; method id
        mov  dword [eax+0xc], {magic:#x}
.fill:
        mov  [eax+8], ecx               ; out_len
        mov  edx, edi                   ; post surface
        mov  esi, ebp                   ; chunk
        lea  edi, [eax+0x10]            ; input slice at block+0x10
        push ecx
        push ecx                        ; out_len x2 across the copies
        push ds
        pop  es
        cld
        mov  ecx, {in_cap:#x}
        rep  movsb
        pop  ecx
        mov  esi, edx
        rep  movsb                      ; post follows the input slice
        pop  ecx
        mov  esi, ebx
        rep  movsb                      ; pre follows the post surface
.done:
        pop  es
        popfd
        popad
        add  esp, 4                     ; discard the call's return address
{displaced}
        jmp  {resume:#x}
"""

_DEFAULT_IMAGE = None


def _default_image():
    """Flat LE image of the retail FIFA96.EXE, for direct build_cave calls."""
    global _DEFAULT_IMAGE
    if _DEFAULT_IMAGE is None:
        data = DEFAULT_ISO.read_bytes()
        lba, size = find_iso_file(data, "FIFA96.EXE")
        _DEFAULT_IMAGE = le.parse(data[lba * SECTOR:lba * SECTOR + size])["image"]
    return _DEFAULT_IMAGE


def find_iso_file(data, name):
    """Return (extent_lba, size) of a root-directory file, case-insensitive.

    Walks the ISO9660 root directory (PVD at sector 16) record by record;
    the optional version suffix (``;1``) is ignored.
    """
    pvd = 16 * SECTOR
    if data[pvd + 1:pvd + 6] != b"CD001":
        raise ValueError("no ISO9660 PVD at sector 16")
    if len(data) < pvd + 190:
        raise ValueError("truncated ISO9660 PVD")
    root = data[pvd + 156:pvd + 156 + 34]
    extent = struct.unpack_from("<I", root, 2)[0]
    size = struct.unpack_from("<I", root, 10)[0]
    query = name.split(";")[0].lower()
    off = extent * SECTOR
    end = off + size
    if end > len(data):
        raise ValueError(
            f"root directory out of range: extent {extent}, size {size}")
    while off < end:
        rec_len = data[off]
        if rec_len == 0:                        # last record of the sector
            off = ((off // SECTOR) + 1) * SECTOR
            continue
        if off + rec_len > end:
            raise ValueError(f"directory record at 0x{off:x} overruns dir")
        name_len = data[off + 32]
        rec_name = data[off + 33:off + 33 + name_len].decode("latin1")
        if rec_name.split(";")[0].lower() == query:
            lba = struct.unpack_from("<I", data, off + 2)[0]
            fsize = struct.unpack_from("<I", data, off + 10)[0]
            return lba, fsize
        off += rec_len
    raise ValueError(f"{name} not found in the ISO root directory")


def link_to_file_offset(info, link):
    """Map a link-time address to its byte offset in the LE page store.

    Rejects links that fall outside every object or past an object's stored
    pages (BSS); the globally last page is short (info["last_page"]).
    """
    page_size = info["page_size"]
    for obj in info["objects"]:
        if obj["base"] <= link < obj["base"] + obj["vsize"]:
            break
    else:
        raise ValueError(f"link {link:#x} is not inside any object")
    page = (link - obj["base"]) // page_size
    if page >= obj["pages"]:
        raise ValueError(
            f"link {link:#x} is in BSS (page {page} >= "
            f"{obj['pages']} stored pages of object at {obj['base']:#x})")
    global_page = obj["page_index"] - 1 + page
    stored = info["last_page"] if global_page == info["pages"] - 1 \
        else page_size
    within = (link - obj["base"]) % page_size
    if within >= stored:
        raise ValueError(f"link {link:#x} is in BSS (past stored page end)")
    off = info["data_offset"]
    for other in info["objects"]:
        if other is obj:
            break
        off += page_size * other["pages"]
    return off + page_size * page + within


def zero_run_length(image, addr):
    """Length of the all-zero run that starts at `addr` (0 if not zero)."""
    if addr < 0 or addr >= len(image) or image[addr] != 0:
        return 0
    n = 0
    while addr + n < len(image) and image[addr + n] == 0:
        n += 1
    return n


def _assemble(source, optimize=False):
    """Assemble a trampoline source string, returning the flat bytes."""
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "cave.asm"
        out = Path(tmp) / "cave.bin"
        src.write_text(source)
        argv = ["nasm"]
        if optimize:
            argv.append("-O9")
        argv += ["-f", "bin", str(src), "-o", str(out)]
        proc = subprocess.run(argv, capture_output=True, text=False)
        if proc.returncode != 0:
            raise RuntimeError(
                "nasm failed: " + proc.stderr.decode("utf-8", "replace"))
        return out.read_bytes()


def overwrite_len(image, target):
    """Length of the whole-instruction prefix to displace at target (>= 5).

    Uses ndisasm to find the first instruction boundary at or past 5 bytes.
    Raises ValueError on a branch opcode in the prefix (a displaced branch
    cannot be re-executed from the cave) or if no boundary is reached in 16
    bytes.
    """
    blob = image[target:target + 32]
    proc = subprocess.run(["ndisasm", "-b32", "-o", hex(target), "-"],
                          input=blob, capture_output=True, text=False)
    if proc.returncode != 0:
        raise RuntimeError(
            "ndisasm failed: " + proc.stderr.decode("utf-8", "replace"))
    instructions = []
    current = None
    for line in proc.stdout.decode("ascii", "replace").splitlines():
        if not line.strip():
            continue
        m = re.match(r"^[0-9A-F]+  ([0-9A-F]+)(?:\s|$)", line)
        if m:                                   # instruction starts here
            if current is not None:
                instructions.append(current)
            current = m.group(1)
            continue
        m = re.match(r"^\s+-([0-9A-F]+)(?:\s|$)", line)
        if m and current is not None:           # wrapped byte continuation
            current += m.group(1)
            continue
        raise ValueError(
            f"unparseable ndisasm output for {target:#x}: {line!r}")
    if current is not None:
        instructions.append(current)
    if not instructions:
        raise ValueError(f"could not disassemble an overwrite prefix at "
                         f"{target:#x}")

    total = 0
    for opcode in instructions:
        if len(opcode) % 2:
            raise ValueError(f"truncated instruction bytes {opcode!r} in "
                             f"the overwrite prefix at {target:#x}")
        first = int(opcode[:2], 16)
        if first in (0x9A, 0xC2, 0xC3, 0xCA, 0xCB,
                     0xE8, 0xE9, 0xEA, 0xEB) or \
                0x70 <= first <= 0x7F or 0xE0 <= first <= 0xE3 or \
                (first == 0x0F and len(opcode) >= 4 and
                 0x80 <= int(opcode[2:4], 16) <= 0x8F):
            raise ValueError(
                f"branch opcode {opcode[:2]} in overwrite prefix at "
                f"{target:#x}")
        total += len(opcode) // 2
        if total >= 5:
            return total
        if total > 16:
            raise ValueError(f"no instruction boundary within 16 bytes "
                             f"of {target:#x}")
    raise ValueError(f"could not disassemble an overwrite prefix at "
                     f"{target:#x}")


def build_cave(target, overwrite, site_id, cave_link, image=None,
               capture_eax=False, capacity=CAVE_CAPACITY):
    """Assemble the trampoline cave, displacing overwrite bytes of target.

    With capture_eax the entry EAX (a mode/state id) is packed into the high
    16 bits of the site word; the low 16 bits stay site_id. `capacity` is the
    writable/zero bytes available at cave_link (default: the FU-11 slot).
    """
    if image is None:
        image = _default_image()
    if overwrite < 5:
        raise ValueError(f"overwrite must cover the 5-byte call, got "
                         f"{overwrite}")
    displaced = "        db " + ",".join(
        f"0x{b:02x}" for b in image[target:target + overwrite])
    if capture_eax:
        es_setup = ("        push ss\n"
                    "        pop  es\n"
                    "        shl  eax, 0x10\n"
                    f"        or   eax, strict dword {site_id:#x}\n"
                    "        mov  esi, eax")
        set_site = ""
    else:
        es_setup = "        mov  ax, ss\n        mov  es, ax"
        set_site = f"        mov  esi, {site_id:#x}"
    source = CAVE_TEMPLATE.format(cave=cave_link, displaced=displaced,
                                  adjust=overwrite - 5,
                                  resume=target + overwrite,
                                  es_setup=es_setup, set_site=set_site)
    cave = _assemble(source)
    if len(cave) > capacity:
        raise ValueError(f"cave is {len(cave)} bytes, over the "
                         f"{capacity:#x} capacity")
    return cave


def build_vgt_entry_cave(target, overwrite, site_id, cave_link, image=None,
                         capacity=CAVE_CAPACITY):
    """Assemble the FU-21 VGT entry descriptor trampoline (T_PROBE site 1).

    Same proven DPMI structure as the FU-11 template, but the caller-address
    split is replaced by the observed method byte (stream[0]) in the
    caller_lo slot and EBX/EDX stay zero from the structure zero-fill.
    `target` is decode_record_dispatch (0x9E718).
    """
    if image is None:
        image = _default_image()
    if overwrite < 5:
        raise ValueError(f"overwrite must cover the 5-byte call, got "
                         f"{overwrite}")
    displaced = "        db " + ",".join(
        f"0x{b:02x}" for b in image[target:target + overwrite])
    source = VGT_ENTRY_TEMPLATE.format(
        cave=cave_link, displaced=displaced, adjust=overwrite - 5,
        resume=target + overwrite, site=site_id)
    cave = _assemble(source, optimize=True)
    if len(cave) > capacity:
        raise ValueError(f"VGT entry cave is {len(cave)} bytes, over the "
                         f"{capacity:#x} capacity")
    return cave


def build_vgt_return_cave(target_ret, overwrite, cave_link, scratch_link,
                          image=None, alloc_link=0x98BF8,
                          in_cap=0x4400, out_cap=0x4000, min_out=0x20,
                          capacity=CAVE_CAPACITY, magic=VGT_MAGIC):
    """Assemble the FU-21 return-side per-method golden-record trampoline.

    Entered from a `call` patched over the dispatcher epilogue at
    `target_ret` (0x9E859). Reads stream/dst from the caller's argument slots
    and ESI (returned decoded size) from the target's saved ESI. For each
    record passing the min_out/out_cap filter whose raw method byte has no
    slot yet, allocates an in_cap+out_cap+16 block from the game heap, links
    it at the head of `scratch_link`'s chain and copies a bounded input slice
    plus the exact output. A slot is never overwritten: the walk skips blocks
    whose method field matches.
    """
    if image is None:
        image = _default_image()
    if overwrite < 5:
        raise ValueError(f"overwrite must cover the 5-byte call, got "
                         f"{overwrite}")
    displaced = "        db " + ",".join(
        f"0x{b:02x}" for b in image[target_ret:target_ret + overwrite])
    source = VGT_RETURN_TEMPLATE.format(
        cave=cave_link, displaced=displaced, call_ret=target_ret + 5,
        resume=target_ret + overwrite, scratch=scratch_link,
        alloc=alloc_link, in_cap=in_cap, out_cap=out_cap, min_out=min_out,
        range_cap=out_cap - min_out,
        alloc_size=0x10 + in_cap + out_cap, magic=magic)
    cave = _assemble(source, optimize=True)
    if len(cave) > capacity:
        raise ValueError(f"VGT return cave is {len(cave)} bytes, over the "
                         f"{capacity:#x} capacity")
    return cave


def build_vgt_capture(entry_target, return_target, site_id, cave_link,
                      image=None, capacity=CAVE_CAPACITY, **return_kw):
    """Build the FU-21 two-trampoline per-method capture blob.

    Layout: [scratch:u32][entry probe][return capture]. The entry probe is
    the FU-11-derived T_PROBE descriptor (site_id) carrying the observed
    method byte; the return capture fills one linked golden slot per distinct
    method. Returns a dict with the blob and its placement.
    """
    if image is None:
        image = _default_image()
    if zero_run_length(image, cave_link) < capacity:
        raise ValueError(f"cave {cave_link:#x} does not have {capacity:#x} "
                         f"zero bytes")
    scratch_link = cave_link
    entry_link = cave_link + 4
    entry_ow = overwrite_len(image, entry_target)
    entry = build_vgt_entry_cave(entry_target, entry_ow, site_id, entry_link,
                                 image=image,
                                 capacity=capacity - 4)
    return_link = entry_link + len(entry)
    ret_ow = overwrite_len(image, return_target)
    ret = build_vgt_return_cave(return_target, ret_ow, return_link,
                                scratch_link, image=image,
                                capacity=capacity - 4 - len(entry),
                                **return_kw)
    blob = b"\x00" * 4 + entry + ret
    if len(blob) > capacity:
        raise ValueError(f"VGT capture blob is {len(blob)} bytes, over the "
                         f"{capacity:#x} capacity")
    return dict(blob=blob, scratch_link=scratch_link, entry_link=entry_link,
                return_link=return_link, entry_len=len(entry),
                return_len=len(ret), entry_ow=entry_ow, ret_ow=ret_ow,
                entry_target=entry_target, return_target=return_target,
                site_id=site_id)


def build_fvgt_return_cave(target_ret, overwrite, cave_link, scratch_link,
                           image=None, alloc_link=0x98BF8,
                           in_cap=FVGT_IN_CAP, out_cap=FVGT_OUT_CAP,
                           capacity=CAVE_CAPACITY, magic=VGT_MAGIC,
                           method=FVGT_METHOD, tag=FVGT_TAG):
    """Assemble the FU-31 fVGT capture trampoline.

    Entered from a `call` patched over the vgt_decode_f epilogue at
    `target_ret` (0xAE20B). The caller frame is still live, so the cave
    reads ctx/chunk from the argument slots and copies the composited
    surface (ctx[0]*ctx[1] bytes at ctx[10]+0x10, index 10 = byte 0x28)
    plus an `in_cap` input slice from the chunk. One block per run: it is
    allocated at `scratch_link` on the first qualifying call and later
    calls overwrite it (capture-latest; a lost magic re-allocates).
    """
    if image is None:
        image = _default_image()
    if overwrite < 5:
        raise ValueError(f"overwrite must cover the 5-byte call, got "
                         f"{overwrite}")
    displaced = "        db " + ",".join(
        f"0x{b:02x}" for b in image[target_ret:target_ret + overwrite])
    source = FVGT_RETURN_TEMPLATE.format(
        cave=cave_link, displaced=displaced, call_ret=target_ret + 5,
        resume=target_ret + overwrite, scratch=scratch_link,
        alloc=alloc_link, in_cap=in_cap, out_cap=out_cap,
        alloc_size=0x10 + in_cap + out_cap, magic=magic, method=method,
        tag=tag)
    cave = _assemble(source, optimize=True)
    if len(cave) > capacity:
        raise ValueError(f"fVGT return cave is {len(cave)} bytes, over the "
                         f"{capacity:#x} capacity")
    return cave


def build_fvgt_capture(return_target, cave_link, image=None,
                       capacity=CAVE_CAPACITY, **return_kw):
    """Build the FU-31 fVGT capture blob: [scratch:u32][return cave].

    The single trampoline shares the FU-20/21 zero run at `cave_link`; the
    scratch head cell stays at the cave link and the call targets cave+4.
    """
    if image is None:
        image = _default_image()
    if zero_run_length(image, cave_link) < capacity:
        raise ValueError(f"cave {cave_link:#x} does not have {capacity:#x} "
                         f"zero bytes")
    scratch_link = cave_link
    return_link = cave_link + 4
    ret_ow = overwrite_len(image, return_target)
    ret = build_fvgt_return_cave(return_target, ret_ow, return_link,
                                 scratch_link, image=image,
                                 capacity=capacity - 4, **return_kw)
    blob = b"\x00" * 4 + ret
    if len(blob) > capacity:
        raise ValueError(f"fVGT capture blob is {len(blob)} bytes, over the "
                         f"{capacity:#x} capacity")
    return dict(blob=blob, scratch_link=scratch_link, return_link=return_link,
                return_len=len(ret), ret_ow=ret_ow,
                return_target=return_target)


def build_fvgt_entry_cave(target, overwrite, cave_link, marker_link,
                          image=None, capacity=None):
    """Assemble the FU-32 entry marker trampoline.

    Entered from a `call` patched over `vgt_decode_f`'s prologue at
    `target` (0xADEFC). Records the pre-call surface pointer (ctx[10]
    before the entry swap, cdecl arg 1) into `marker_link` through the
    load delta recovered from the call's return address. Clobbers only
    EAX (caller-saved); ESI is restored, flags are left to the displaced
    prologue. `capacity` defaults to the zero run after the marker cell.
    """
    if image is None:
        image = _default_image()
    if overwrite < 5:
        raise ValueError(f"overwrite must cover the 5-byte call, got "
                         f"{overwrite}")
    if capacity is None:
        capacity = zero_run_length(image, marker_link) - 4
    displaced = "        db " + ",".join(
        f"0x{b:02x}" for b in image[target:target + overwrite])
    source = FVGT_ENTRY_TEMPLATE.format(
        cave=cave_link, displaced=displaced, call_ret=target + 5,
        resume=target + overwrite, marker=marker_link)
    cave = _assemble(source, optimize=True)
    if len(cave) > capacity:
        raise ValueError(f"fVGT entry cave is {len(cave)} bytes, over the "
                         f"{capacity:#x} capacity")
    return cave


def build_fvgt_pair_cave(target_ret, overwrite, cave_link, scratch_link,
                         marker_link, image=None, alloc_link=0x98BF8,
                         in_cap=FVGT_IN_CAP, out_cap=FVGT_OUT_CAP,
                         capacity=CAVE_CAPACITY, magic=VGT_MAGIC,
                         method=FVGT_METHOD, tag=FVGT_TAG):
    """Assemble the FU-32 paired pre/post capture trampoline.

    Entered from a `call` patched over `vgt_decode_f`'s epilogue at
    `target_ret` (0xAE20B). The caller frame is still live, so the cave
    reads ctx/chunk from the argument slots. It copies an `in_cap` chunk
    slice followed by the post surface (ctx[10]+0x10) and the pre surface
    (the `marker_link` pointer recorded by the entry hook, +0x10) into
    one block: 0x10 header + in_cap + 2*out_len. Capture-latest: every
    later qualifying call overwrites the block in place; a lost magic
    re-allocates. `next` is left untouched (pair extraction does not walk
    a chain).
    """
    if image is None:
        image = _default_image()
    if overwrite < 5:
        raise ValueError(f"overwrite must cover the 5-byte call, got "
                         f"{overwrite}")
    displaced = "        db " + ",".join(
        f"0x{b:02x}" for b in image[target_ret:target_ret + overwrite])
    source = FVGT_PAIR_TEMPLATE.format(
        cave=cave_link, displaced=displaced, call_ret=target_ret + 5,
        resume=target_ret + overwrite, scratch=scratch_link,
        marker=marker_link, alloc=alloc_link, in_cap=in_cap, out_cap=out_cap,
        alloc_size=0x10 + in_cap + 2 * out_cap, magic=magic, method=method,
        tag=tag)
    cave = _assemble(source, optimize=True)
    if len(cave) > capacity:
        raise ValueError(f"fVGT pair cave is {len(cave)} bytes, over the "
                         f"{capacity:#x} capacity")
    return cave


def build_fvgt_pair_capture(entry_target, return_target, entry_cave_link,
                            return_cave_link, image=None, entry_capacity=None,
                            return_capacity=CAVE_CAPACITY, **return_kw):
    """Build the FU-32 two-point fVGT pair capture blobs.

    Layouts: entry run = [marker:u32][entry cave], return run =
    [scratch:u32][return cave]. `entry_capacity` defaults to the measured
    zero run at `entry_cave_link`; `return_capacity` defaults to the
    FU-20/21/31 run size (0x103) and is zero-run checked.
    """
    if image is None:
        image = _default_image()
    if entry_capacity is None:
        entry_capacity = zero_run_length(image, entry_cave_link)
    if entry_capacity < 4:
        raise ValueError(f"entry cave {entry_cave_link:#x} has no zero run")
    if zero_run_length(image, return_cave_link) < return_capacity:
        raise ValueError(f"cave {return_cave_link:#x} does not have "
                         f"{return_capacity:#x} zero bytes")
    marker_link = entry_cave_link
    entry_link = entry_cave_link + 4
    entry_ow = overwrite_len(image, entry_target)
    entry = build_fvgt_entry_cave(entry_target, entry_ow, entry_link,
                                  marker_link, image=image,
                                  capacity=entry_capacity - 4)
    scratch_link = return_cave_link
    return_link = return_cave_link + 4
    ret_ow = overwrite_len(image, return_target)
    ret = build_fvgt_pair_cave(return_target, ret_ow, return_link,
                               scratch_link, marker_link, image=image,
                               capacity=return_capacity - 4, **return_kw)
    entry_blob = b"\x00" * 4 + entry
    ret_blob = b"\x00" * 4 + ret
    if len(entry_blob) > entry_capacity:
        raise ValueError(f"fVGT pair entry blob is {len(entry_blob)} bytes, "
                         f"over the {entry_capacity:#x} capacity")
    if len(ret_blob) > return_capacity:
        raise ValueError(f"fVGT pair return blob is {len(ret_blob)} bytes, "
                         f"over the {return_capacity:#x} capacity")
    return dict(entry_blob=entry_blob, ret_blob=ret_blob,
                entry_len=len(entry), return_len=len(ret),
                marker_link=marker_link, entry_link=entry_link,
                scratch_link=scratch_link, return_link=return_link,
                entry_ow=entry_ow, ret_ow=ret_ow,
                entry_target=entry_target, return_target=return_target)


def rel32(to, next_ip):
    """Signed 32-bit displacement to `to` from the instruction end `next_ip`."""
    return struct.pack("<i", to - next_ip)


def patch_iso(data, target, cave, site_id, capture_eax=False,
              capacity=CAVE_CAPACITY):
    """Return a copy of the ISO with the trampoline applied to FIFA96.EXE.

    The output has the ISO's original size; the input bytes are untouched.
    """
    lba, size = find_iso_file(data, "FIFA96.EXE")
    base = lba * SECTOR
    exe = data[base:base + size]
    info = le.parse(exe)
    ow = overwrite_len(info["image"], target)
    cave_bytes = build_cave(target, ow, site_id, cave, image=info["image"],
                            capture_eax=capture_eax, capacity=capacity)
    target_off = link_to_file_offset(info, target)
    link_to_file_offset(info, target + 4)
    cave_off = link_to_file_offset(info, cave)
    link_to_file_offset(info, cave + len(cave_bytes) - 1)
    if any(info["image"][cave:cave + len(cave_bytes)]):
        raise ValueError(f"cave {cave:#x} is not zero-filled; refusing to "
                         f"overwrite live bytes")
    out = bytearray(data)
    out[base + cave_off:base + cave_off + len(cave_bytes)] = cave_bytes
    out[base + target_off:base + target_off + 5] = \
        b"\xe8" + rel32(cave, target + 5)
    return bytes(out)


def patch_iso_vgt(data, entry_target, return_target, cave, site_id,
                  capacity=CAVE_CAPACITY, **return_kw):
    """Return (patched ISO bytes, placement) for the FU-20 capture blob.

    A 5-byte `call` is patched over `entry_target` (0x9E718, entry probe)
    and over `return_target` (0x9E859, golden capture); the shared blob
    [scratch][entry][return] is written over the zero run at `cave`.
    """
    lba, size = find_iso_file(data, "FIFA96.EXE")
    base = lba * SECTOR
    exe = data[base:base + size]
    info = le.parse(exe)
    built = build_vgt_capture(entry_target, return_target, site_id, cave,
                              image=info["image"], capacity=capacity,
                              **return_kw)
    blob = built["blob"]
    cave_off = link_to_file_offset(info, cave)
    link_to_file_offset(info, cave + len(blob) - 1)
    if any(info["image"][cave:cave + len(blob)]):
        raise ValueError(f"cave {cave:#x} is not zero-filled; refusing to "
                         f"overwrite live bytes")
    calls = []
    for target, link in ((entry_target, built["entry_link"]),
                         (return_target, built["return_link"])):
        off = link_to_file_offset(info, target)
        link_to_file_offset(info, target + 4)
        calls.append((off, b"\xe8" + rel32(link, target + 5)))
    out = bytearray(data)
    out[base + cave_off:base + cave_off + len(blob)] = blob
    for off, call in calls:
        out[base + off:base + off + 5] = call
    return bytes(out), built


def patch_iso_fvgt(data, return_target, cave, capacity=CAVE_CAPACITY, **kw):
    """Return (patched ISO bytes, placement) for the FU-31 fVGT capture.

    A 5-byte `call` is patched over `return_target` (0xAE20B) and the blob
    [scratch][return cave] is written over the zero run at `cave`.
    """
    lba, size = find_iso_file(data, "FIFA96.EXE")
    base = lba * SECTOR
    exe = data[base:base + size]
    info = le.parse(exe)
    built = build_fvgt_capture(return_target, cave, image=info["image"],
                               capacity=capacity, **kw)
    blob = built["blob"]
    cave_off = link_to_file_offset(info, cave)
    link_to_file_offset(info, cave + len(blob) - 1)
    if any(info["image"][cave:cave + len(blob)]):
        raise ValueError(f"cave {cave:#x} is not zero-filled; refusing to "
                         f"overwrite live bytes")
    off = link_to_file_offset(info, return_target)
    link_to_file_offset(info, return_target + 4)
    out = bytearray(data)
    out[base + cave_off:base + cave_off + len(blob)] = blob
    out[base + off:base + off + 5] = \
        b"\xe8" + rel32(built["return_link"], return_target + 5)
    return bytes(out), built


def patch_iso_fvgt_pair(data, entry_target, return_target, entry_cave,
                        return_cave, entry_capacity=None,
                        return_capacity=CAVE_CAPACITY, **kw):
    """Return (patched ISO bytes, placement) for the FU-32 pair capture.

    A 5-byte `call` is patched over the entry target (0xADEFC) and over the
    return target (0xAE20B); the marker blob is written over the zero run
    at `entry_cave` and the scratch+pair cave blob over the run at
    `return_cave`.
    """
    lba, size = find_iso_file(data, "FIFA96.EXE")
    base = lba * SECTOR
    exe = data[base:base + size]
    info = le.parse(exe)
    if entry_capacity is None:
        entry_capacity = zero_run_length(info["image"], entry_cave)
    built = build_fvgt_pair_capture(
        entry_target, return_target, entry_cave, return_cave,
        image=info["image"], entry_capacity=entry_capacity,
        return_capacity=return_capacity, **kw)
    writes = []
    for cave, blob in ((entry_cave, built["entry_blob"]),
                       (return_cave, built["ret_blob"])):
        cave_off = link_to_file_offset(info, cave)
        link_to_file_offset(info, cave + len(blob) - 1)
        if any(info["image"][cave:cave + len(blob)]):
            raise ValueError(f"cave {cave:#x} is not zero-filled; refusing "
                             f"to overwrite live bytes")
        writes.append((cave_off, blob))
    calls = []
    for target, link in ((entry_target, built["entry_link"]),
                         (return_target, built["return_link"])):
        off = link_to_file_offset(info, target)
        link_to_file_offset(info, target + 4)
        calls.append((off, b"\xe8" + rel32(link, target + 5)))
    out = bytearray(data)
    for off, blob in writes:
        out[base + off:base + off + len(blob)] = blob
    for off, call in calls:
        out[base + off:base + off + 5] = call
    return bytes(out), built


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--iso", required=True, help="input ISO (read-only)")
    ap.add_argument("--out", help="patched ISO copy (must differ from --iso)")
    ap.add_argument("--target", required=True, type=lambda s: int(s, 0),
                    help="target entry link address")
    ap.add_argument("--cave", type=lambda s: int(s, 0),
                    help="cave link address")
    ap.add_argument("--site-id", type=lambda s: int(s, 0))
    ap.add_argument("--capture-eax", action="store_true",
                    help="pack the entry EAX (mode) into the site word")
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument("--vgt-capture", action="store_true",
                      help="patch the FU-20 two-point VGT golden capture "
                           "(entry probe + return capture) instead of a "
                           "single trampoline")
    mode.add_argument("--fvgt-capture", action="store_true",
                      help="patch the FU-31 fVGT single-frame capture "
                           "(return cave at --target only)")
    mode.add_argument("--fvgt-pair", action="store_true",
                      help="patch the FU-32 paired fVGT pre/post capture "
                           "(entry marker + pair return cave)")
    ap.add_argument("--entry-cave", type=lambda s: int(s, 0),
                    help="entry marker cave link address for --fvgt-pair")
    ap.add_argument("--return-target", type=lambda s: int(s, 0),
                    default=0x9E859,
                    help="return-hook link address for --vgt-capture")
    ap.add_argument("--input-cap", type=lambda s: int(s, 0), default=None,
                    help="input slice cap (default: 0x4400 vgt, 0x4000 fvgt)")
    ap.add_argument("--output-cap", type=lambda s: int(s, 0), default=None,
                    help="output cap/filter (default: 0x4000 vgt, 0x40000 "
                         "fvgt)")
    ap.add_argument("--cave-capacity", type=lambda s: int(s, 0), default=None,
                    help="bytes available at --cave (default: 0x103, or the "
                         "measured zero run with --vgt-capture)")
    ap.add_argument("--print-overwrite", action="store_true",
                    help="print the decimal overwrite length and exit; "
                         "writes no file")
    args = ap.parse_args(argv)

    needs_site = not args.vgt_capture and not args.fvgt_capture \
        and not args.fvgt_pair
    if not args.print_overwrite and (
            not args.out or args.cave is None or
            (args.fvgt_pair and args.entry_cave is None) or
            (needs_site and args.site_id is None)):
        ap.error("--out and --cave (plus --site-id unless --vgt-capture, "
                 "--fvgt-capture or --fvgt-pair; plus --entry-cave with "
                 "--fvgt-pair) are required unless --print-overwrite is "
                 "given")

    src = Path(args.iso)
    try:
        data = src.read_bytes()
        lba, size = find_iso_file(data, "FIFA96.EXE")
        info = le.parse(data[lba * SECTOR:lba * SECTOR + size])
        ow = overwrite_len(info["image"], args.target)
        if args.print_overwrite:
            print(ow)
            return 0
        if args.fvgt_pair:
            in_cap = (FVGT_IN_CAP if args.input_cap is None
                      else args.input_cap)
            out_cap = (FVGT_OUT_CAP if args.output_cap is None
                       else args.output_cap)
            return_capacity = args.cave_capacity
            if return_capacity is None:
                return_capacity = zero_run_length(info["image"], args.cave)
            entry_capacity = zero_run_length(info["image"], args.entry_cave)
            out, built = patch_iso_fvgt_pair(
                data, args.target, args.return_target, args.entry_cave,
                args.cave, entry_capacity=entry_capacity,
                return_capacity=return_capacity, in_cap=in_cap,
                out_cap=out_cap)
        elif args.fvgt_capture:
            capacity = args.cave_capacity
            if capacity is None:
                capacity = zero_run_length(info["image"], args.cave)
            in_cap = (FVGT_IN_CAP if args.input_cap is None
                      else args.input_cap)
            out_cap = (FVGT_OUT_CAP if args.output_cap is None
                       else args.output_cap)
            out, built = patch_iso_fvgt(
                data, args.target, args.cave, capacity=capacity,
                in_cap=in_cap, out_cap=out_cap)
        elif args.vgt_capture:
            capacity = args.cave_capacity
            if capacity is None:
                capacity = zero_run_length(info["image"], args.cave)
            in_cap = 0x4400 if args.input_cap is None else args.input_cap
            out_cap = 0x4000 if args.output_cap is None else args.output_cap
            out, built = patch_iso_vgt(
                data, args.target, args.return_target, args.cave,
                args.site_id, capacity=capacity, in_cap=in_cap,
                out_cap=out_cap)
        else:
            capacity = args.cave_capacity or CAVE_CAPACITY
            out = patch_iso(data, args.target, args.cave, args.site_id,
                            capture_eax=args.capture_eax, capacity=capacity)
            built = None
    except (ValueError, RuntimeError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    dst = Path(args.out)
    if src.resolve() == dst.resolve():
        print("error: --out must not overwrite --iso", file=sys.stderr)
        return 1
    dst.write_bytes(out)
    if args.fvgt_pair:
        print(f"fvgt entry {args.target:#x} overwrite: "
              f"{built['entry_ow']} bytes (resume "
              f"{args.target + built['entry_ow']:#x})")
        print(f"fvgt return {args.return_target:#x} overwrite: "
              f"{built['ret_ow']} bytes (resume "
              f"{args.return_target + built['ret_ow']:#x})")
        print(f"entry cave {args.entry_cave:#x}: marker "
              f"{built['marker_link']:#x}, entry {built['entry_link']:#x} "
              f"({built['entry_len']} B), blob {len(built['entry_blob'])} B")
        print(f"return cave {args.cave:#x}: scratch "
              f"{built['scratch_link']:#x}, return {built['return_link']:#x} "
              f"({built['return_len']} B), blob {len(built['ret_blob'])} B")
    elif args.fvgt_capture:
        print(f"fvgt return {args.target:#x} overwrite: "
              f"{built['ret_ow']} bytes (resume {args.target + built['ret_ow']:#x})")
        print(f"cave {args.cave:#x}: scratch {built['scratch_link']:#x}, "
              f"return {built['return_link']:#x} ({built['return_len']} B), "
              f"blob {len(built['blob'])} B")
    elif built is not None:
        print(f"vgt entry {args.target:#x} overwrite: "
              f"{built['entry_ow']} bytes")
        print(f"vgt return {args.return_target:#x} overwrite: "
              f"{built['ret_ow']} bytes")
        print(f"cave {args.cave:#x}: scratch {built['scratch_link']:#x}, "
              f"entry {built['entry_link']:#x} ({built['entry_len']} B), "
              f"return {built['return_link']:#x} ({built['return_len']} B), "
              f"blob {len(built['blob'])} B")
    else:
        cave_len = len(build_cave(
            args.target, ow, args.site_id, args.cave, image=info["image"],
            capture_eax=args.capture_eax, capacity=capacity))
        print(f"target {args.target:#x} overwrite: {ow} bytes")
        print(f"cave {args.cave:#x}: {cave_len} bytes")
    print(f"file-size delta: {len(out) - len(data)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
