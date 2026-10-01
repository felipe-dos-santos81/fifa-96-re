import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fifa96_patch as patch  # noqa: E402
import fifa96_le as le  # noqa: E402

ISO = ROOT / "game" / "FIFAPCCD96.iso"
TARGET = 0x9E718
CAVE = 0x6728D


class TestIso(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.iso = ISO.read_bytes()

    def test_find_fifa96_exe(self):
        lba, size = patch.find_iso_file(self.iso, "FIFA96.EXE")
        self.assertGreater(lba, 16)
        self.assertGreater(size, 0x170000)
        exe = self.iso[lba * 2048:lba * 2048 + size]
        self.assertEqual(exe[:2], b"MZ")
        info = le.parse(exe)
        self.assertEqual(info["header_offset"], 0x290A4)

    def test_patch_changes_only_target_and_cave(self):
        out = patch.patch_iso(self.iso, TARGET, CAVE, 1)
        self.assertEqual(len(out), len(self.iso))
        diffs = [i for i in range(len(out)) if out[i] != self.iso[i]]
        self.assertGreater(len(diffs), 5)
        # every changed byte lies in one of the two patched regions
        lba, size = patch.find_iso_file(self.iso, "FIFA96.EXE")
        base = lba * 2048
        exe = bytearray(self.iso[base:base + size])
        off_t = patch.link_to_file_offset(le.parse(bytes(exe)), TARGET)
        off_c = patch.link_to_file_offset(le.parse(bytes(exe)), CAVE)
        for d in diffs:
            rel = d - base
            self.assertTrue(off_t <= rel < off_t + 8 or
                            off_c <= rel < off_c + 0x103,
                            f"unexpected diff at file offset 0x{rel:x}")


class TestCave(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.iso = ISO.read_bytes()
        lba, size = patch.find_iso_file(cls.iso, "FIFA96.EXE")
        cls.info = le.parse(cls.iso[lba * 2048:lba * 2048 + size])

    def test_overwrite_len_prefix(self):
        self.assertEqual(patch.overwrite_len(self.info["image"], TARGET), 7)
        self.assertEqual(patch.overwrite_len(self.info["image"], 0x9E860), 6)
        self.assertEqual(patch.overwrite_len(self.info["image"], 0xC9D10), 6)
        self.assertEqual(patch.overwrite_len(self.info["image"], 0xCAD30), 6)

    def test_overwrite_len_counts_wrapped_instruction(self):
        # ndisasm prints the first 8 bytes on the instruction line and wraps
        # the rest onto a continuation line; the 10-byte mov must count whole
        image = b"\xc7\x05\x00\x10\x00\x00\x78\x56\x34\x12" + b"\x90" * 22
        self.assertEqual(patch.overwrite_len(image, 0), 10)

    def test_cave_ebp_reports_resume_address(self):
        # the patched `call cave` pushes target+5, so the cave must add
        # overwrite-5 to reach the resume address in the EBP slot
        for ow in (7, 5):
            cave = patch.build_cave(TARGET, ow, 1, CAVE)
            i = cave.find(b"\x83\xc5")          # add ebp, imm8
            self.assertGreaterEqual(
                i, 0, f"no `add ebp, imm8` in the overwrite={ow} cave")
            self.assertEqual(cave[i + 2], ow - 5,
                             f"EBP adjust wrong for overwrite={ow}")

    def test_cave_contains_displaced_bytes_and_jump(self):
        ow = patch.overwrite_len(self.info["image"], TARGET)
        cave = patch.build_cave(TARGET, ow, 1, CAVE)
        self.assertLess(len(cave), 0x103)
        displaced = self.info["image"][TARGET:TARGET + ow]
        tgt = TARGET + ow
        jmp_off = cave.rindex(b"\xe9")
        rel = struct.unpack_from("<i", cave, jmp_off + 1)[0]
        self.assertEqual(CAVE + jmp_off + 5 + rel, tgt)
        self.assertIn(displaced, cave)


if __name__ == "__main__":
    unittest.main()
