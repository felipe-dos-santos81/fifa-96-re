"""Unit tests for tools/fifa96_le.py — synthetic LE plus the retail build.

Run: python3 tests/test_le.py
"""
import importlib.util
import os
import struct
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location(
    "fifa96_le", os.path.join(ROOT, "tools", "fifa96_le.py"))
le = importlib.util.module_from_spec(spec)
spec.loader.exec_module(le)

RETAIL = "/media/felipe/FIFAPCCD/fifa96.exe"


def build_synthetic(hdr_off=0x400, page_map=None, pages=2, last=0x100):
    data = bytearray(hdr_off + 0x200)
    data[0:2] = b"MZ"
    data[hdr_off:hdr_off + 2] = b"LE"
    put = lambda fmt, off, val: struct.pack_into(fmt, data, hdr_off + off, val)
    put("<H", 0x08, 2)
    put("<H", 0x0A, 1)
    put("<I", 0x14, pages)
    put("<I", 0x18, 1)
    put("<I", 0x1C, 0x42)
    put("<I", 0x20, 1)
    put("<I", 0x24, 0x80)
    put("<I", 0x28, 0x1000)
    put("<I", 0x2C, last)
    put("<I", 0x40, 0x100)
    put("<I", 0x44, 1)
    put("<I", 0x48, 0x80)
    put("<I", 0x68, 0x180)
    struct.pack_into("<IIIIII", data, hdr_off + 0x100,
                     0x1234, 0x10000, 0x2005, 1, pages, 0)
    def encode(n):
        return (((n & 0xFF) << 8 | (n >> 8)) << 8)

    values = page_map if page_map is not None else [
        encode(i + 1) for i in range(pages)]
    for i, v in enumerate(values):
        struct.pack_into("<I", data, hdr_off + 0x80 + 4 * i, v)
    content = bytes((i * 7) & 0xFF for i in range(0x1000 + last))
    data += content
    return bytes(data), content


class SyntheticTest(unittest.TestCase):
    def test_layout(self):
        data, content = build_synthetic()
        info = le.parse(data)
        self.assertEqual(info["pages"], 2)
        self.assertEqual(info["data_offset"], len(data) - (0x1000 + 0x100))
        self.assertEqual(info["entry"], 0x10042)
        self.assertEqual(info["stack"], 0x10080)
        self.assertEqual(len(info["image"]), 0x11234)
        self.assertEqual(info["objects"][0]["base"], 0x10000)
        self.assertEqual(info["image"][0x10042:0x1004A], content[0x42:0x4A])

    def test_non_sequential_page_map_rejected(self):
        data, _ = build_synthetic(page_map=[1 << 16, 5 << 16])
        with self.assertRaises(ValueError):
            le.parse(data)

    def test_no_le_header(self):
        with self.assertRaises(ValueError):
            le.parse(b"MZ" + b"\x00" * 64)


class RetailTest(unittest.TestCase):
    @unittest.skipUnless(os.path.exists(RETAIL), "retail FIFA96.EXE not present")
    def test_retail_layout(self):
        with open(RETAIL, "rb") as fh:
            data = fh.read()
        info = le.parse(data)
        self.assertEqual(info["header_offset"], 0x290A4)
        self.assertEqual(info["pages"], 263)
        self.assertEqual(info["last_page"], 0xFD7)
        self.assertEqual(info["data_offset"], 0x6DA54)
        self.assertEqual([o["base"] for o in info["objects"]],
                         [0x10000, 0xE0000, 0xF0000, 0x100000])
        self.assertEqual([o["pages"] for o in info["objects"]],
                         [194, 1, 1, 67])
        self.assertEqual(len(info["image"]), 0x16AA50)
        self.assertEqual(info["entry"], 0x9FD10)
        self.assertEqual(info["image"][0x9FD10:0x9FD12], b"\xeb\x76")
        self.assertEqual(info["image"][0x10000 + 0xC1EA0:0x10000 + 0xC1EA0 + 8],
                         b"\x00" * 8)


if __name__ == "__main__":
    unittest.main()
