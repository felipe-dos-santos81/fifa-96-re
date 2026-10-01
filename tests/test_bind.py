"""Unit tests for tools/fifa96_bind.py — synthetic trace frames + in-memory library.

Run: python3 tests/test_bind.py
"""
import importlib.util
import os
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location(
    "fifa96_bind", os.path.join(ROOT, "tools", "fifa96_bind.py"))
bind = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bind)


def put(b, n, t, seq, payload):
    b[n[0]] = t
    b[n[0] + 1] = seq & 0xFF
    b[n[0] + 2] = (seq >> 8) & 0xFF
    b[n[0] + 3] = len(payload) & 0xFF
    b[n[0] + 4] = (len(payload) >> 8) & 0xFF
    b[n[0] + 5:n[0] + 5 + len(payload)] = payload
    n[0] += 5 + len(payload)


def file_record(ah, bx, cx, ds, dx, ax_after, flags, h, data):
    p = bytearray()
    p.append(ah)
    p += bx.to_bytes(2, "little")
    p += cx.to_bytes(2, "little")
    p += ds.to_bytes(2, "little")
    p += dx.to_bytes(2, "little")
    p += ax_after.to_bytes(2, "little")
    p.append(flags)
    p += h.to_bytes(4, "little")
    p += len(data).to_bytes(2, "little")
    p += data
    return bytes(p)


def open_record(name, handle):
    return file_record(0x3D, 0, 0, 0, 0, handle, 0x02, 0, name.encode())


def read_record(handle, requested, got, h, head):
    return file_record(0x3F, handle, requested, 0, 0, got, 0x05, h, head)


class FnvTest(unittest.TestCase):
    def test_known_vectors(self):
        self.assertEqual(bind.fnv1a32(b""), 2166136261)
        self.assertEqual(bind.fnv1a32(b"a"), 0xE40C292C)
        self.assertEqual(bind.fnv1a32(b"foobar"), 0xBF9CF968)


class ParseTest(unittest.TestCase):
    def test_parse_open_and_read(self):
        buf = bytearray(256)
        n = [0]
        put(buf, n, 1, 0, b"FCAP\x01\x05")
        data = b"HELLO WORLD"
        put(buf, n, 2, 0, open_record("D:\\SOUND\\X.BN", 5))
        put(buf, n, 2, 1, read_record(5, len(data), len(data),
                                      bind.fnv1a32(data), data))
        recs = bind.parse_trace(bytes(buf[:n[0]]))
        self.assertEqual(len(recs), 2)
        self.assertEqual(recs[0]["ah"], 0x3D)
        self.assertEqual(recs[0]["ax_after"], 5)
        self.assertTrue(recs[0]["is_name"])
        self.assertEqual(recs[1]["ah"], 0x3F)
        self.assertEqual(recs[1]["data"], data)


class BindTest(unittest.TestCase):
    CONTENT = b"0123456789HELLO WORLD THIS IS A TEST FILE."

    def library(self):
        return {"/SOUND/X.BN": self.CONTENT}

    def resolve(self, recs):
        return bind.resolve_handles(recs)

    def test_bind_first_chunk(self):
        recs = bind.parse_trace(self._stream(
            [open_record("D:\\SOUND\\X.BN", 5),
             read_record(5, 11, 11, bind.fnv1a32(self.CONTENT[:11]),
                         self.CONTENT[:11])]))
        names = self.resolve(recs)
        got = bind.bind_read(recs[1], names, self.library())
        self.assertEqual(got, ("/SOUND/X.BN", 0, 11))

    def test_bind_after_seek(self):
        off = 10
        n = 11
        recs = bind.parse_trace(self._stream(
            [open_record("D:\\SOUND\\X.BN", 5),
             read_record(5, n, n, bind.fnv1a32(self.CONTENT[off:off + n]),
                         self.CONTENT[off:off + n])]))
        names = self.resolve(recs)
        got = bind.bind_read(recs[1], names, self.library())
        self.assertEqual(got, ("/SOUND/X.BN", off, n))

    def test_bad_hash_unbound(self):
        recs = bind.parse_trace(self._stream(
            [open_record("D:\\SOUND\\X.BN", 5),
             read_record(5, 5, 5, 0xDEADBEEF, self.CONTENT[:5])]))
        names = self.resolve(recs)
        self.assertIsNone(bind.bind_read(recs[1], names, self.library()))

    def test_unknown_handle_unbound(self):
        recs = bind.parse_trace(self._stream(
            [read_record(9, 5, 5, bind.fnv1a32(self.CONTENT[:5]),
                         self.CONTENT[:5])]))
        names = self.resolve(recs)
        self.assertIsNone(bind.bind_read(recs[0], names, self.library()))

    def test_failed_open_does_not_bind_name(self):
        failed = file_record(0x3D, 0, 0, 0, 0, 2, 0x00, 0, b"")
        recs = bind.parse_trace(self._stream(
            [failed,
             read_record(2, 5, 5, bind.fnv1a32(self.CONTENT[:5]),
                         self.CONTENT[:5])]))
        names = self.resolve(recs)
        self.assertEqual(names, {})
        self.assertIsNone(bind.bind_read(recs[1], names, self.library()))

    def test_sequential_bind_reused_handle(self):
        other = b"SECOND FILE CONTENT"
        lib = {"/SOUND/X.BN": self.CONTENT, "/SOUND/Y.BN": other}
        recs = bind.parse_trace(self._stream([
            open_record("D:\\SOUND\\X.BN", 5),
            read_record(5, 5, 5, bind.fnv1a32(self.CONTENT[:5]),
                        self.CONTENT[:5]),
            file_record(0x3E, 5, 0, 0, 0, 0, 0, 0, b""),
            open_record("D:\\SOUND\\Y.BN", 5),
            read_record(5, 6, 6, bind.fnv1a32(other[:6]), other[:6]),
        ]))
        res = bind.sequential_bind(recs, lib)
        self.assertEqual(res["reads"], 2)
        self.assertEqual(res["bound"], 2)
        self.assertEqual(res["by_name"]["D:\\SOUND\\X.BN"]["bound"], 1)
        self.assertEqual(res["by_name"]["D:\\SOUND\\Y.BN"]["bound"], 1)
        self.assertEqual(res["by_name"]["D:\\SOUND\\Y.BN"]["iso"],
                         {"/SOUND/Y.BN"})

    def _stream(self, payloads):
        buf = bytearray(4096)
        n = [0]
        put(buf, n, 1, 0, b"FCAP\x01\x05")
        for i, p in enumerate(payloads):
            put(buf, n, 2, i, p)
        return bytes(buf[:n[0]])


if __name__ == "__main__":
    unittest.main()
