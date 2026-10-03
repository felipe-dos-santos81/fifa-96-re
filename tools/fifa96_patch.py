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
    ap.add_argument("--vgt-capture", action="store_true",
                    help="patch the FU-20 two-point VGT golden capture "
                         "(entry probe + return capture) instead of a single "
                         "trampoline")
    ap.add_argument("--return-target", type=lambda s: int(s, 0),
                    default=0x9E859,
                    help="return-hook link address for --vgt-capture")
    ap.add_argument("--input-cap", type=lambda s: int(s, 0), default=0x4400,
                    help="input slice cap for --vgt-capture")
    ap.add_argument("--output-cap", type=lambda s: int(s, 0), default=0x4000,
                    help="output cap/filter for --vgt-capture")
    ap.add_argument("--cave-capacity", type=lambda s: int(s, 0), default=None,
                    help="bytes available at --cave (default: 0x103, or the "
                         "measured zero run with --vgt-capture)")
    ap.add_argument("--print-overwrite", action="store_true",
                    help="print the decimal overwrite length and exit; "
                         "writes no file")
    args = ap.parse_args(argv)

    if not args.print_overwrite and (
            not args.out or args.cave is None or args.site_id is None):
        ap.error("--out, --cave and --site-id are required unless "
                 "--print-overwrite is given")

    src = Path(args.iso)
    try:
        data = src.read_bytes()
        lba, size = find_iso_file(data, "FIFA96.EXE")
        info = le.parse(data[lba * SECTOR:lba * SECTOR + size])
        ow = overwrite_len(info["image"], args.target)
        if args.print_overwrite:
            print(ow)
            return 0
        if args.vgt_capture:
            capacity = args.cave_capacity
            if capacity is None:
                capacity = zero_run_length(info["image"], args.cave)
            out, built = patch_iso_vgt(
                data, args.target, args.return_target, args.cave,
                args.site_id, capacity=capacity, in_cap=args.input_cap,
                out_cap=args.output_cap)
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
    if built is not None:
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
