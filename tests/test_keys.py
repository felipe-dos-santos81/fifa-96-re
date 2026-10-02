import io
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fifa96_keys as keys  # noqa: E402


class TestParse(unittest.TestCase):
    def test_comments_and_blanks_skipped(self):
        self.assertEqual(keys.parse_keys("# c\n\n  5 enter\n"),
                         [(5.0, "enter")])

    def test_int_and_float_waits(self):
        self.assertEqual(keys.parse_keys("5 enter\n2.5 esc\n"),
                         [(5.0, "enter"), (2.5, "esc")])

    def test_multi_key_line_kept_verbatim(self):
        self.assertEqual(keys.parse_keys("3 up enter , kp_8\n"),
                         [(3.0, "up enter , kp_8")])

    def test_missing_keys_raises_with_line(self):
        with self.assertRaises(keys.KeysError) as cm:
            keys.parse_keys("5\n")
        self.assertIn("line 1", str(cm.exception))

    def test_invalid_wait_raises(self):
        with self.assertRaises(keys.KeysError):
            keys.parse_keys("soon enter\n")

    def test_negative_wait_raises(self):
        with self.assertRaises(keys.KeysError):
            keys.parse_keys("-1 enter\n")


class TestAutotype(unittest.TestCase):
    def test_cumulative_waits_preserve_order(self):
        steps = keys.parse_keys("8 enter\n4 esc\n")
        self.assertEqual(keys.autotype_lines(steps),
                         ["AUTOTYPE -w 8 -p 0.1 enter",
                          "AUTOTYPE -w 12 -p 0.1 esc"])

    def test_pace_override(self):
        steps = keys.parse_keys("2 enter\n")
        self.assertEqual(keys.autotype_lines(steps, pace=0.3),
                         ["AUTOTYPE -w 2 -p 0.3 enter"])


class TestCli(unittest.TestCase):
    def _write(self, text):
        fh = tempfile.NamedTemporaryFile("w", suffix=".keys", delete=False)
        fh.write(text)
        fh.close()
        self.addCleanup(Path(fh.name).unlink)
        return fh.name

    def _run(self, argv):
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            rc = keys.main(argv)
        return rc, out.getvalue(), err.getvalue()

    def test_print_output(self):
        rc, out, _ = self._run([self._write("5 enter\n")])
        self.assertEqual((rc, out), (0, "AUTOTYPE -w 5 -p 0.1 enter\n"))

    def test_check_ok_is_silent(self):
        rc, out, _ = self._run(["--check", self._write("5 enter\n")])
        self.assertEqual((rc, out), (0, ""))

    def test_check_bad_reports_line(self):
        rc, out, err = self._run(["--check", self._write("nope\n")])
        self.assertEqual(rc, 1)
        self.assertIn("line 1", err)

    def test_bad_pace_reports(self):
        rc, _, err = self._run(["--pace", "fast", self._write("5 enter\n")])
        self.assertEqual(rc, 1)
        self.assertIn("--pace", err)

    def test_missing_file_reports(self):
        rc, _, err = self._run(["/nonexistent.keys"])
        self.assertEqual(rc, 1)
        self.assertIn("error:", err)


if __name__ == "__main__":
    unittest.main()
