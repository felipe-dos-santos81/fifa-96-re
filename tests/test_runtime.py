"""Unit tests for tools/fifa96_runtime.py — synthetic guest dump.

Run: python3 tests/test_runtime.py
"""
import importlib.util
import os
import struct
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location(
    "fifa96_runtime", os.path.join(ROOT, "tools", "fifa96_runtime.py"))
rt = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rt)


def build_dump(delta, include_obj2=True):
    dump = bytearray(0x800000)
    dump[rt.ENTRY_LINK + delta:rt.ENTRY_LINK + delta + 2] = b"\xeb\x76"
    dump[rt.BANNER_LINK + delta:rt.BANNER_LINK + delta + len(rt.BANNER)] = rt.BANNER
    if include_obj2:
        dump[rt.OBJ2_LINK + delta:rt.OBJ2_LINK + delta + 8] = rt.OBJ2_MAGIC
        dump[rt.OBJ3_LINK + delta:rt.OBJ3_LINK + delta + 8] = rt.OBJ3_MAGIC
    return bytes(dump)


class DeltaTest(unittest.TestCase):
    def test_recover_delta(self):
        delta = 0x210000
        d, anchors = rt.find_delta(build_dump(delta))
        self.assertEqual(d, delta)
        self.assertTrue(anchors["entry_bytes"])
        self.assertTrue(anchors["obj2"])
        self.assertTrue(anchors["obj3"])

    def test_missing_banner(self):
        delta, anchors = rt.find_delta(bytes(0x1000))
        self.assertIsNone(delta)

    def test_runtime_addresses(self):
        delta = 0x1000
        self.assertEqual(rt.ADDRESSES["vgt_dispatch"] + delta, 0xAF4BC)
        self.assertEqual(rt.ADDRESSES["game_read_int21"] + delta, 0xBBFB8)


if __name__ == "__main__":
    unittest.main()
