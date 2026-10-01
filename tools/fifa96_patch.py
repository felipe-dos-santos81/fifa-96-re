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
        mov  ax, ss
        mov  es, ax
        mov  ebx, [esp+44]          ; caller return address (relocated)
        mov  ebp, [esp+40]          ; call's return (target+5, relocated)
        add  ebp, {adjust}          ; resume is target+overwrite
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
    root = data[pvd + 156:pvd + 156 + 34]
    extent = struct.unpack_from("<I", root, 2)[0]
    size = struct.unpack_from("<I", root, 10)[0]
    query = name.split(";")[0].lower()
    off = extent * SECTOR
    end = off + size
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
        if first in (0xE8, 0xE9, 0xEB) or 0x70 <= first <= 0x7F or \
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


def build_cave(target, overwrite, site_id, cave_link, image=None):
    """Assemble the trampoline cave, displacing overwrite bytes of target."""
    if image is None:
        image = _default_image()
    if overwrite < 5:
        raise ValueError(f"overwrite must cover the 5-byte call, got "
                         f"{overwrite}")
    displaced = "        db " + ",".join(
        f"0x{b:02x}" for b in image[target:target + overwrite])
    source = CAVE_TEMPLATE.format(cave=cave_link, site=site_id,
                                  displaced=displaced,
                                  adjust=overwrite - 5,
                                  resume=target + overwrite)
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "cave.asm"
        out = Path(tmp) / "cave.bin"
        src.write_text(source)
        proc = subprocess.run(["nasm", "-f", "bin", str(src), "-o", str(out)],
                              capture_output=True, text=False)
        if proc.returncode != 0:
            raise RuntimeError(
                "nasm failed: " + proc.stderr.decode("utf-8", "replace"))
        cave = out.read_bytes()
    if len(cave) > CAVE_CAPACITY:
        raise ValueError(f"cave is {len(cave)} bytes, over the "
                         f"{CAVE_CAPACITY:#x} capacity")
    return cave


def rel32(to, next_ip):
    """Signed 32-bit displacement to `to` from the instruction end `next_ip`."""
    return struct.pack("<i", to - next_ip)


def patch_iso(data, target, cave, site_id):
    """Return a copy of the ISO with the trampoline applied to FIFA96.EXE.

    The output has the ISO's original size; the input bytes are untouched.
    """
    lba, size = find_iso_file(data, "FIFA96.EXE")
    base = lba * SECTOR
    exe = data[base:base + size]
    info = le.parse(exe)
    ow = overwrite_len(info["image"], target)
    cave_bytes = build_cave(target, ow, site_id, cave, image=info["image"])
    target_off = link_to_file_offset(info, target)
    link_to_file_offset(info, target + 4)
    cave_off = link_to_file_offset(info, cave)
    link_to_file_offset(info, cave + len(cave_bytes) - 1)
    out = bytearray(data)
    out[base + cave_off:base + cave_off + len(cave_bytes)] = cave_bytes
    out[base + target_off:base + target_off + 5] = \
        b"\xe8" + rel32(cave, target + 5)
    return bytes(out)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--iso", required=True, help="input ISO (read-only)")
    ap.add_argument("--out", required=True,
                    help="patched ISO copy (must differ from --iso)")
    ap.add_argument("--target", required=True, type=lambda s: int(s, 0),
                    help="target entry link address")
    ap.add_argument("--cave", required=True, type=lambda s: int(s, 0),
                    help="cave link address")
    ap.add_argument("--site-id", required=True, type=lambda s: int(s, 0))
    args = ap.parse_args(argv)

    src, dst = Path(args.iso), Path(args.out)
    if src.resolve() == dst.resolve():
        print("error: --out must not overwrite --iso", file=sys.stderr)
        return 1
    try:
        data = src.read_bytes()
        out = patch_iso(data, args.target, args.cave, args.site_id)
        lba, size = find_iso_file(data, "FIFA96.EXE")
        info = le.parse(data[lba * SECTOR:lba * SECTOR + size])
        ow = overwrite_len(info["image"], args.target)
        cave_len = len(build_cave(args.target, ow, args.site_id, args.cave,
                                  image=info["image"]))
    except (ValueError, RuntimeError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    dst.write_bytes(out)
    print(f"target {args.target:#x} overwrite: {ow} bytes")
    print(f"cave {args.cave:#x}: {cave_len} bytes")
    print(f"file-size delta: {len(out) - len(data)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
