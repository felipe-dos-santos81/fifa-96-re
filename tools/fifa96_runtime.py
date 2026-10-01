#!/usr/bin/env python3
"""fifa96_runtime.py — map link-time LE image addresses to runtime addresses.

The game runs the protected-mode image relocated by a constant delta
(runtime = link + delta) discovered from a guest-RAM dump. This tool
recovers that delta and prints the runtime addresses of the known anchors
(VGT decoders, file-I/O wrappers) so a debugger breakpoint can be set.

Usage:
  fifa96_runtime.py --delta 0x1FC010
  fifa96_runtime.py guest.bin

The WATCOM run-time banner string is the primary dump anchor (link flat
0x9FD12, preceded by the entry jmp). Object 2/3 anchors (link 0xE0000 and
0xF0000) are reported when those pages are resident; they can legitimately be
absent in a demand-paged dump, so a successful entry-bytes check is the
primary signal.
"""
import argparse
import struct
import sys

# anchor in the link-time flat image
BANNER = b"WATCOM C/C++32 Run-Time system"
BANNER_LINK = 0x9FD12
ENTRY_LINK = 0x9FD10
OBJ2_LINK = 0xE0000
OBJ2_MAGIC = bytes.fromhex("5052b020e620ba2e")
OBJ3_LINK = 0xF0000
OBJ3_MAGIC = bytes.fromhex("878381820a0a0a0a")

ADDRESSES = {
    "entry": 0x9FD10,
    "obj2_stub": 0xE0000,
    "obj3_data": 0xF0000,
    "obj4_base": 0x100000,
    "vgt_stream_poll": 0x67BA8,
    "vgt_dispatch": 0xAE4BC,
    "vgt_decode_f": 0xADEFC,
    "vgt_decode_k": 0xAE218,
    "file_open_ro": 0xBABE0,
    "game_open_int21": 0xBAC10,
    "game_read_wrapper": 0xBAFB1,
    "game_read_int21": 0xBAFB8,
    "watcom_read_int21": 0xCD666,
    "fread": 0xAEFBA,
}


def find_delta(dump):
    """Return (delta, anchors) or (None, anchors) if no banner copy is found."""
    anchors = {}
    pos = dump.find(BANNER)
    delta = None
    if pos >= 0:
        delta = pos - BANNER_LINK
        anchors["entry_bytes"] = dump[ENTRY_LINK + delta:
                                      ENTRY_LINK + delta + 2] == b"\xeb\x76"
    anchors["obj2"] = OBJ2_MAGIC in dump and delta is not None and \
        dump[OBJ2_LINK + delta:OBJ2_LINK + delta + 8] == OBJ2_MAGIC
    anchors["obj3"] = OBJ3_MAGIC in dump and delta is not None and \
        dump[OBJ3_LINK + delta:OBJ3_LINK + delta + 8] == OBJ3_MAGIC
    if delta is not None and not (0 <= delta < len(dump)):
        return None, anchors
    return delta, anchors


def dump_copies(dump, needle):
    out = []
    i = 0
    while True:
        i = dump.find(needle, i)
        if i < 0:
            return out
        out.append(i)
        i += 1


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("dump", nargs="?", help="guest RAM dump to locate the delta")
    ap.add_argument("--delta", help="explicit relocation delta (hex or dec)")
    args = ap.parse_args(argv)

    if args.delta:
        delta = int(args.delta, 0)
        anchors = {"explicit": True}
    elif args.dump:
        dump = open(args.dump, "rb").read()
        delta, anchors = find_delta(dump)
        copies = dump_copies(dump, BANNER)
        print(f"banner copies at: {[hex(c) for c in copies]}")
        print(f"anchors: {anchors}")
        if delta is None:
            print("no banner copy found", file=sys.stderr)
            return 1
    else:
        ap.error("give a dump path or --delta")

    print(f"delta = {delta:#x} (page aligned: {delta % 0x1000 == 0})")
    print(f"{'name':22} {'link':>9} {'runtime':>12}")
    for name, link in ADDRESSES.items():
        print(f"{name:22} {link:#9x} {link + delta:#12x}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
