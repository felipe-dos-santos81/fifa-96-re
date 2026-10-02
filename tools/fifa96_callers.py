#!/usr/bin/env python3
"""fifa96_callers.py — static direct-call census over the FIFA96 LE image.

Scans the code object of the extracted flat image for `E8 rel32` instructions
whose target is the requested link address, and reports the call-opcode
address plus the return address (opcode + 5).

The DPMI probe reports runtime RETURN addresses, so cross-check a runtime
`caller_link` against `ret` here, never against `opcode`.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fifa96_le as le  # noqa: E402
import fifa96_patch as patch  # noqa: E402

CODE_START = 0x10000
CODE_END = 0x10000 + 0xC1EA0  # obj1: base 0x10000, vsize 0xC1EA0 (LE parse)
DEFAULT_ISO = Path(__file__).resolve().parents[1] / "game" / "FIFAPCCD96.iso"


def load_image(iso_path):
    """Extract the flat LE image from FIFA96.EXE inside the ISO."""
    data = Path(iso_path).read_bytes()
    lba, size = patch.find_iso_file(data, "FIFA96.EXE")
    return le.parse(data[lba * 2048:lba * 2048 + size])["image"]


def direct_callers(image, target, start=CODE_START, end=CODE_END):
    """Return E8 callers of `target` as [{'opcode': int, 'ret': int}]."""
    out = []
    target &= 0xFFFFFFFF
    end = min(end, len(image) - 5)
    for i in range(start, end):
        if image[i] != 0xE8:
            continue
        rel = struct.unpack_from("<i", image, i + 1)[0]
        ret = i + 5
        if (ret + rel) & 0xFFFFFFFF == target:
            out.append({"opcode": i, "ret": ret})
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description="static direct-call census")
    ap.add_argument("target", type=lambda s: int(s, 0))
    ap.add_argument("--iso", default=str(DEFAULT_ISO))
    args = ap.parse_args(argv)
    image = load_image(args.iso)
    got = direct_callers(image, args.target)
    for c in got:
        print(f'opcode=0x{c["opcode"]:x} ret=0x{c["ret"]:x}')
    print(f"callers={len(got)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
