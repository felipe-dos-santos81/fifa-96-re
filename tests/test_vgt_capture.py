import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fifa96_le as le  # noqa: E402
import fifa96_patch as patch  # noqa: E402
import fifa96_probe as probe  # noqa: E402
import fifa96_runtime as rt  # noqa: E402
import fifa96_vgt_capture as vgt  # noqa: E402

ISO = ROOT / "game" / "FIFAPCCD96.iso"
ENTRY = 0x9E718
RETURN = 0x9E859
CAVE = 0x6728D
GOLDEN = ROOT / "tests" / "golden" / "vgt"


def tprobe(site, caller=0x29A871, target_ret=0x29A71F):
    payload = struct.pack("<IIII", site, caller & 0xFFFF,
                          (caller >> 16) & 0xFFFF, target_ret)
    return bytes([probe.T_PROBE]) + struct.pack("<HH", 0, 16) + payload


def synthetic_dump(scratch=0x400000, delta_load=0x1FC000,
                   delta_dump=0x1FC010, method=0x6A, out_len=0x100,
                   magic=vgt.VGT_MAGIC, corrupt=None):
    size = 0x500000
    data = bytearray(size)
    banner_at = rt.BANNER_LINK + delta_dump
    data[banner_at:banner_at + len(rt.BANNER)] = rt.BANNER
    block_off = scratch + delta_dump - delta_load
    struct.pack_into("<I", data, vgt.SCRATCH_LINK + delta_dump, scratch)
    inp = bytearray(vgt.IN_CAP)
    inp[0] = method
    inp[1] = 0xFB
    inp[2] = (out_len >> 16) & 0xFF
    inp[3] = (out_len >> 8) & 0xFF
    inp[4] = out_len & 0xFF
    for i in range(5, len(inp)):
        inp[i] = (i * 7) & 0xFF
    out = bytes((i * 13 + 1) & 0xFF for i in range(out_len))
    hdr = struct.pack("<I", magic) + bytes([method, 0xFB, 0, 0]) + \
        struct.pack("<I", out_len)
    data[block_off:block_off + 0x10] = hdr
    data[block_off + 0x10:block_off + 0x10 + len(inp)] = inp
    data[block_off + 0x10 + vgt.IN_CAP:
         block_off + 0x10 + vgt.IN_CAP + out_len] = out
    if corrupt == "zero_scratch":
        struct.pack_into("<I", data, vgt.SCRATCH_LINK + delta_dump, 0)
    elif corrupt == "magic":
        struct.pack_into("<I", data, block_off, 0xDEADBEEF)
    elif corrupt == "length":
        data[block_off + 0x10 + 4] ^= 0xFF
    return bytes(data)


class TestTemplate(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.iso = ISO.read_bytes()
        lba, size = patch.find_iso_file(cls.iso, "FIFA96.EXE")
        cls.info = le.parse(cls.iso[lba * 2048:lba * 2048 + size])

    def test_zero_run_length(self):
        img = self.info["image"]
        self.assertEqual(patch.zero_run_length(img, CAVE), 0x103)
        self.assertEqual(patch.zero_run_length(img, CAVE + 0x102), 1)
        self.assertEqual(patch.zero_run_length(img, CAVE + 0x103), 0)
        self.assertEqual(patch.zero_run_length(img, 0x10000), 0)

    def test_capture_fits_the_cave(self):
        built = patch.build_vgt_capture(ENTRY, RETURN, 1, CAVE,
                                        image=self.info["image"])
        self.assertEqual(built["entry_ow"], 7)
        self.assertEqual(built["ret_ow"], 5)
        self.assertEqual(built["scratch_link"], CAVE)
        self.assertEqual(built["entry_link"], CAVE + 4)
        self.assertEqual(built["return_link"], CAVE + 4 + built["entry_len"])
        self.assertLessEqual(len(built["blob"]), 0x103)
        self.assertEqual(len(built["blob"]),
                         4 + built["entry_len"] + built["return_len"])

    def test_capture_capacity_enforced(self):
        with self.assertRaises(ValueError):
            patch.build_vgt_capture(ENTRY, RETURN, 1, CAVE,
                                    image=self.info["image"], capacity=0x40)

    def test_return_cave_delta_uses_call_return(self):
        # the patched `call` pushes target+5, so the delta subtraction must
        # use 0x9E85E, not the 0x9E859 overwrite address
        cave = patch.build_vgt_return_cave(RETURN, 5, 0x100000, CAVE,
                                           image=self.info["image"])
        self.assertIn(b"\x81\xee" + struct.pack("<I", RETURN + 5), cave)
        self.assertNotIn(b"\x81\xee" + struct.pack("<I", RETURN), cave)

    def test_return_cave_reads_saved_esi_not_flags(self):
        # the decoded length is the pushad-saved ESI at [esp+0x0c]; [esp+4]
        # is the pushed EFLAGS word (the diag run caught this)
        cave = patch.build_vgt_return_cave(RETURN, 5, 0x100000, CAVE,
                                           image=self.info["image"])
        self.assertIn(b"\x8b\x4c\x24\x0c", cave)     # mov ecx,[esp+0xc]
        self.assertNotIn(b"\x8b\x4c\x24\x04", cave)  # mov ecx,[esp+4]

    def test_return_cave_replays_epilogue_and_allocates(self):
        cave = patch.build_vgt_return_cave(RETURN, 5, 0x100000, CAVE,
                                           image=self.info["image"])
        displaced = self.info["image"][RETURN:RETURN + 5]
        self.assertIn(displaced, cave)
        self.assertIn(b"\x46\x56\x47\x54", cave)      # 'FVGT' magic
        # direct, position-independent call to FUN_00098bf8 (a `call [mem]`
        # would jump through the function's own code bytes)
        targets = [0x100000 + i + 5 + struct.unpack_from("<i", cave, i + 1)[0]
                   for i in range(len(cave) - 4) if cave[i] == 0xE8]
        self.assertIn(0x98BF8, targets)
        self.assertNotIn(b"\xff\x96", cave)           # no indirect call
        # debug-tag pointer is the zeroed scratch cell, not link 0x33CC
        # (whose runtime fixup carries a second relocation delta)
        self.assertIn(b"\x8d\x86" + struct.pack("<I", CAVE), cave)
        jmp = cave.rindex(b"\xe9")
        rel = struct.unpack_from("<i", cave, jmp + 1)[0]
        self.assertEqual(0x100000 + jmp + 5 + rel, RETURN + 5)

    def test_patch_iso_vgt_changes_only_three_regions(self):
        out, built = patch.patch_iso_vgt(self.iso, ENTRY, RETURN, CAVE, 1)
        self.assertEqual(len(out), len(self.iso))
        lba, size = patch.find_iso_file(self.iso, "FIFA96.EXE")
        base = lba * 2048
        image = self.info["image"]
        off_e = patch.link_to_file_offset(self.info, ENTRY)
        off_r = patch.link_to_file_offset(self.info, RETURN)
        off_c = patch.link_to_file_offset(self.info, CAVE)
        diffs = [i for i in range(len(out)) if out[i] != self.iso[i]]
        self.assertGreater(len(diffs), 5)
        for d in diffs:
            rel = d - base
            self.assertTrue(
                off_e <= rel < off_e + 5 or
                off_r <= rel < off_r + 5 or
                off_c <= rel < off_c + len(built["blob"]),
                f"unexpected diff at file offset 0x{rel:x}")
        # both call sites point at their trampolines
        for target, link, off in ((ENTRY, built["entry_link"], off_e),
                                  (RETURN, built["return_link"], off_r)):
            call = out[base + off:base + off + 5]
            self.assertEqual(call[0], 0xE8)
            disp = struct.unpack("<i", call[1:])[0]
            self.assertEqual(target + 5 + disp, link)
        self.assertEqual(image[CAVE:CAVE + 4], b"\x00" * 4)


class TestProbeHelpers(unittest.TestCase):
    def test_site_frames_and_load_delta(self):
        frames = probe.probe_frames(tprobe(1) + tprobe(2))
        self.assertEqual(len(probe.site_frames(frames, 1)), 1)
        self.assertEqual(probe.site_frames(frames, 3), [])
        self.assertEqual(
            probe.load_delta(frames, ENTRY, vgt.ENTRY_OVERWRITE),
            0x29A71F - (ENTRY + 7))
        self.assertIsNone(probe.load_delta([], ENTRY, 7))


class TestExtract(unittest.TestCase):
    def test_extract_writes_successful_record(self):
        record = vgt.extract(synthetic_dump(), tprobe(1))
        self.assertEqual(record["method"], 0x6A)
        self.assertEqual(record["out_len"], 0x100)
        self.assertEqual(record["header24"], 0x100)
        self.assertEqual(record["delta_load"], 0x1FC000)
        self.assertNotIn(True, [not v for v in record["checks"].values()])

    def test_extract_requires_live_frames(self):
        with self.assertRaisesRegex(ValueError, "descriptor frames"):
            vgt.extract(synthetic_dump(), tprobe(2))

    def test_extract_rejects_zero_scratch(self):
        with self.assertRaisesRegex(ValueError, "scratch pointer is zero"):
            vgt.extract(synthetic_dump(corrupt="zero_scratch"), tprobe(1))

    def test_extract_rejects_bad_magic(self):
        with self.assertRaisesRegex(ValueError, "magic"):
            vgt.extract(synthetic_dump(corrupt="magic"), tprobe(1))

    def test_extract_rejects_length_mismatch(self):
        with self.assertRaisesRegex(ValueError, "success bar"):
            vgt.extract(synthetic_dump(corrupt="length"), tprobe(1))

    def test_write_golden_roundtrip(self):
        import tempfile
        record = vgt.extract(synthetic_dump(), tprobe(1))
        with tempfile.TemporaryDirectory() as tmp:
            stem = vgt.write_golden(record, tmp)
            self.assertEqual((Path(tmp) / "record-6a.in.bin").read_bytes(),
                             record["input"])
            self.assertEqual((Path(tmp) / "record-6a.out.bin").read_bytes(),
                             record["output"])
            self.assertTrue((Path(tmp) / "record-6a.json").exists())
            self.assertEqual(stem.name, "record-6a")


class TestGoldenVectors(unittest.TestCase):
    def test_committed_vectors_pass_success_bar(self):
        vectors = sorted(GOLDEN.glob("record-*.in.bin"))
        if not vectors:
            self.skipTest("no golden vectors captured yet")
        for inp_path in vectors:
            with self.subTest(vector=inp_path.name):
                stem = inp_path.with_suffix("").with_suffix("")
                out_path = stem.with_suffix(".out.bin")
                meta = stem.with_suffix(".json")
                self.assertTrue(out_path.exists())
                self.assertTrue(meta.exists())
                inp = inp_path.read_bytes()
                out = out_path.read_bytes()
                self.assertGreaterEqual(len(inp), 5)
                self.assertEqual(inp[1], 0xFB)
                self.assertIn(inp[0] & 0xFE, vgt.KNOWN_METHODS)
                self.assertLessEqual(len(out), vgt.OUT_CAP)
                self.assertGreaterEqual(len(out), vgt.MIN_OUT)


if __name__ == "__main__":
    unittest.main()
