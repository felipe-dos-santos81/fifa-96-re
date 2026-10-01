import struct
import sys
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


if __name__ == "__main__":
    unittest.main()
