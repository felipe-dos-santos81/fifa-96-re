"""Unit tests for tools/fifa96_frames.py — no ffmpeg/identify required.

Run: python3 tests/test_frames.py
"""
import io
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fifa96_frames as frames  # noqa: E402


class TestFindAvis(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)

    def test_recursive_and_sorted(self):
        (self.root / "video").mkdir()
        (self.root / "video" / "b.avi").write_bytes(b"")
        (self.root / "a.avi").write_bytes(b"")
        (self.root / "video" / "sub").mkdir()
        (self.root / "video" / "sub" / "c.avi").write_bytes(b"")
        (self.root / "notes.txt").write_text("not a video")
        self.assertEqual(frames.find_avis(self.root),
                         [self.root / "a.avi",
                          self.root / "video" / "b.avi",
                          self.root / "video" / "sub" / "c.avi"])


class TestFfmpegCmd(unittest.TestCase):
    def test_exact_list(self):
        avi = Path("/s/video/clip.avi")
        out_pattern = Path("/s/frames/clip-%04d.png")
        self.assertEqual(
            frames.ffmpeg_cmd(avi, out_pattern, 2.5),
            ["ffmpeg", "-v", "error", "-y", "-i", "/s/video/clip.avi",
             "-vf", "fps=2.5", "/s/frames/clip-%04d.png"])

    def test_integer_rate_has_no_decimal_point(self):
        cmd = frames.ffmpeg_cmd(Path("a.avi"), Path("out/a-%04d.png"), 1.0)
        self.assertIn("fps=1", cmd)


class TestParseIdentifyMean(unittest.TestCase):
    def test_first_number_with_trailing(self):
        self.assertEqual(frames.parse_identify_mean("12345.6 678"), 12345.6)

    def test_multiline(self):
        self.assertEqual(frames.parse_identify_mean("warning\n42.25\n"), 42.25)

    def test_no_number_is_none(self):
        self.assertIsNone(frames.parse_identify_mean("no numbers here"))


class TestMain(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)

    def _run(self, argv):
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            rc = frames.main(argv)
        return rc, out.getvalue(), err.getvalue()

    def test_list_prints_sorted_avis(self):
        (self.root / "video").mkdir()
        (self.root / "video" / "b.avi").write_bytes(b"")
        (self.root / "a.avi").write_bytes(b"")
        rc, out, err = self._run([str(self.root), "--list"])
        self.assertEqual(rc, 0)
        self.assertEqual(err, "")
        self.assertEqual(out, f"{self.root / 'a.avi'}\n"
                              f"{self.root / 'video' / 'b.avi'}\n")

    def test_missing_dir_exits_1(self):
        rc, out, err = self._run([str(self.root / "nope")])
        self.assertEqual(rc, 1)
        self.assertEqual(out, "")
        self.assertIn("error:", err)

    def test_no_avis_exits_1(self):
        rc, out, err = self._run([str(self.root)])
        self.assertEqual(rc, 1)
        self.assertEqual(out, "")
        self.assertIn("error:", err)


if __name__ == "__main__":
    unittest.main()
