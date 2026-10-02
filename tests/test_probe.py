import contextlib
import io
import struct
import sys
import tempfile
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
        self.assertEqual(n["delta"], 0x312000)
        self.assertEqual(n["caller_link"], 0x6BE32)
        self.assertEqual(n["target_ret_link"], 0x9E71F)


class TestSplitSiteMode(unittest.TestCase):
    def test_packed_site_word(self):
        self.assertEqual(probe.split_site_mode(0x000A0002), (2, 10))

    def test_unpacked_site_word_has_mode_zero(self):
        self.assertEqual(probe.split_site_mode(9), (9, 0))


class TestCli(unittest.TestCase):
    def test_truncated_trace_reports_error_without_traceback(self):
        with tempfile.TemporaryDirectory() as tmp:
            trace = Path(tmp) / "trace.bin"
            trace.write_bytes(frame(0x08, b"B" * 16)[:-3])
            err = io.StringIO()
            with contextlib.redirect_stderr(err):
                rc = probe.main([str(trace)])
        self.assertEqual(rc, 1)
        self.assertIn("error:", err.getvalue())
        self.assertNotIn("Traceback", err.getvalue())

    def test_capture_eax_decodes_site_and_mode(self):
        payload = struct.pack("<IIII", 0x000A0002, 0xDE32, 0x0037, 0x3B071F)
        with tempfile.TemporaryDirectory() as tmp:
            trace = Path(tmp) / "trace.bin"
            trace.write_bytes(frame(0x08, payload))
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                rc = probe.main([str(trace), "--capture-eax",
                                 "--target-link", "0x9E718",
                                 "--overwrite", "7"])
        self.assertEqual(rc, 0)
        self.assertIn("site=2 mode=10", buf.getvalue())
        self.assertIn("delta=0x312000", buf.getvalue())

    def test_capture_eax_expect_site_matches_unpacked_id(self):
        payload = struct.pack("<IIII", 0x000A0002, 0xDE32, 0x0037, 0x3B071F)
        with tempfile.TemporaryDirectory() as tmp:
            trace = Path(tmp) / "trace.bin"
            trace.write_bytes(frame(0x08, payload))
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                rc = probe.main([str(trace), "--capture-eax",
                                 "--expect-site", "2"])
            self.assertEqual(rc, 0)
            self.assertIn("hit=True", buf.getvalue())
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                rc = probe.main([str(trace), "--capture-eax",
                                 "--expect-site", "0x000A0002"])
            self.assertEqual(rc, 1)
            self.assertIn("hit=False", buf.getvalue())

    def test_default_expect_site_matches_raw_word(self):
        payload = struct.pack("<IIII", 0x000A0002, 0xDE32, 0x0037, 0x3B071F)
        with tempfile.TemporaryDirectory() as tmp:
            trace = Path(tmp) / "trace.bin"
            trace.write_bytes(frame(0x08, payload))
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                rc = probe.main([str(trace), "--expect-site", "0x000A0002"])
            self.assertEqual(rc, 0)
            self.assertIn("hit=True", buf.getvalue())

    def test_default_print_keeps_raw_site_word(self):
        payload = struct.pack("<IIII", 0x000A0002, 0xDE32, 0x0037, 0x3B071F)
        with tempfile.TemporaryDirectory() as tmp:
            trace = Path(tmp) / "trace.bin"
            trace.write_bytes(frame(0x08, payload))
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                rc = probe.main([str(trace)])
        self.assertEqual(rc, 0)
        self.assertIn("site=655362", buf.getvalue())
        self.assertNotIn("mode=", buf.getvalue())


if __name__ == "__main__":
    unittest.main()
