#!/usr/bin/env python3
"""fifa96_le.py — extract the protected-mode image from FIFA96.EXE.

FIFA96.EXE is a Watcom DOS/4GW linear-executable (LE) file. The MZ image that
Ghidra previously analysed covers only the DOS/4GW loader; the game itself is
the LE image appended to it. This tool rebuilds the flat link-time image the
LE header describes, so it can be imported as x86 32-bit code.

Layout facts used here (verified against the retail build):
  * LE header at file offset 0x290A4 (found by signature scan).
  * Object table at header+0xC4: 4 objects (code, two runtime stubs, stack).
  * Object page map at header+0x124: one dword per page (263 pages). The
    dwords are (logical_page+1)<<16, i.e. an identity mapping, so the pages
    are stored sequentially in object order.
  * The page store ends exactly at EOF: base = filesize - page_bytes, with
    page_bytes = (pages-1)*page_size + bytes_on_last_page. For this build
    base == 0x6DA54 and page_bytes == 0x106FD7.

The output image places each object at its LE RelocBaseAddress:
  obj1 code  @ 0x010000 (0xC1EA0 bytes, 194 pages)
  obj2 stub  @ 0x0E0000 (0x34 bytes, 1 page)
  obj3 data  @ 0x0F0000 (0x18 bytes, 1 page)
  obj4 stack @ 0x100000 (0x6AA50 bytes, 67 stored pages, rest zero-fill)
Entry point (EIP object 1 + EIP) is 0x9FD10 in this address space: a
two-byte jump to the real startup code at 0x9FD88.

Usage:
  fifa96_le.py /path/to/FIFA96.EXE [-o build/fifa96_le.bin] [--info]
"""
import argparse
import struct
import sys

MZ_HEADER_PARAS = 0x200
PAGE_SIZE_DEFAULT = 4096


def find_le_header(data):
    """Return the file offset of the first valid LE header."""
    start = 0
    while True:
        off = data.find(b"LE", start)
        if off < 0:
            return None
        start = off + 1
        if off + 0x84 > len(data):
            continue
        byte_order, word_order = data[off + 2], data[off + 3]
        cpu, os_type = struct.unpack_from("<HH", data, off + 8)
        page_size = struct.unpack_from("<I", data, off + 0x28)[0]
        pages = struct.unpack_from("<I", data, off + 0x14)[0]
        obj_count = struct.unpack_from("<I", data, off + 0x44)[0]
        obj_off = struct.unpack_from("<I", data, off + 0x40)[0]
        if byte_order or word_order:
            continue
        if not (1 <= cpu <= 4 and 0 <= os_type <= 5):
            continue
        if page_size not in (512, 1024, 2048, 4096, 8192) or not (1 <= pages < 65536):
            continue
        if not (1 <= obj_count <= 64):
            continue
        if not (0 < obj_off < len(data) - off):
            continue
        return off
    return None


def parse(data):
    """Parse the LE header, objects and page map. Raises ValueError if invalid."""
    hdr = find_le_header(data)
    if hdr is None:
        raise ValueError("no valid LE header found")
    g = lambda fmt, off: struct.unpack_from(fmt, data, hdr + off)[0]
    pages = g("<I", 0x14)
    page_size = g("<I", 0x28)
    last_page = g("<I", 0x2C)
    eip_obj = g("<I", 0x18)
    eip = g("<I", 0x1C)
    esp_obj = g("<I", 0x20)
    esp = g("<I", 0x24)
    obj_off = g("<I", 0x40)
    obj_count = g("<I", 0x44)
    page_map_off = g("<I", 0x48)

    objects = []
    for i in range(obj_count):
        vsize, base, flags, page_idx, page_count, _ = struct.unpack_from(
            "<IIIIII", data, hdr + obj_off + 24 * i)
        objects.append(dict(vsize=vsize, base=base, flags=flags,
                            page_index=page_idx, pages=page_count))

    # Entries encode byteswap16(logical_page) << 8; the store is sequential.
    for i, value in enumerate(
            (g("<I", page_map_off + 4 * i) for i in range(pages))):
        logical = (((value >> 8) & 0xFF) << 8) | ((value >> 16) & 0xFF)
        if logical != i + 1:
            raise ValueError(
                f"page map entry {i} encodes page {logical}, not {i + 1}")
    expected_first = 1
    for obj in objects:
        if obj["page_index"] != expected_first:
            raise ValueError("object page indices are not contiguous")
        expected_first += obj["pages"]
    if expected_first - 1 != pages:
        raise ValueError("object page counts do not sum to the page count")

    page_bytes = (pages - 1) * page_size + last_page
    data_off = len(data) - page_bytes
    tables_end = hdr + g("<I", 0x68) + (pages + 1) * 4
    if data_off < tables_end:
        raise ValueError("page store overlaps the fixup page table")

    image_size = max(obj["base"] + obj["vsize"] for obj in objects)
    image = bytearray(image_size)
    cursor = data_off
    page_no = 0
    for obj in objects:
        for k in range(obj["pages"]):
            size = page_size if page_no < pages - 1 else last_page
            dst = obj["base"] + k * page_size
            image[dst:dst + size] = data[cursor:cursor + size]
            cursor += size
            page_no += 1
    if cursor != len(data):
        raise ValueError("page store did not consume the file tail")

    return dict(header_offset=hdr, pages=pages, page_size=page_size,
                last_page=last_page, data_offset=data_off,
                image=bytes(image), objects=objects,
                entry=objects[eip_obj - 1]["base"] + eip,
                stack=objects[esp_obj - 1]["base"] + esp)


DEFAULT_EXE = "/media/felipe/FIFAPCCD/fifa96.exe"


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("exe", nargs="?", default=DEFAULT_EXE,
                    help="path to FIFA96.EXE (default: %(default)s)")
    ap.add_argument("-o", "--out", help="write the flat image to this file")
    ap.add_argument("--info", action="store_true", help="print layout summary")
    args = ap.parse_args(argv)

    with open(args.exe, "rb") as fh:
        data = fh.read()
    info = parse(data)

    if args.info or not args.out:
        print(f"file            {args.exe} ({len(data)} bytes)")
        print(f"LE header       {info['header_offset']:#x}")
        print(f"pages           {info['pages']} x {info['page_size']} "
              f"(last {info['last_page']})")
        print(f"page store      {info['data_offset']:#x} .. {len(data):#x}")
        for i, obj in enumerate(info["objects"], 1):
            print(f"object {i}        base={obj['base']:#08x} "
                  f"vsize={obj['vsize']:#x} flags={obj['flags']:#06x} "
                  f"pages={obj['pages']}")
        print(f"entry           {info['entry']:#x}")
        print(f"stack           {info['stack']:#x}")
    if args.out:
        with open(args.out, "wb") as fh:
            fh.write(info["image"])
        print(f"wrote {args.out} ({len(info['image'])} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
