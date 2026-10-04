import hashlib
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fifa96_fvgt_extract as fx  # noqa: E402
import fifa96_runtime as rt  # noqa: E402

GOLDEN = ROOT / "tests" / "golden" / "vgt"
DELTA_LOAD = 0x1FC000
DELTA_DUMP = 0x1FC010
HEAP = 0x400000


def chunk_bytes(index_count=80, raw_count=0, palette_count=352, row_bits=9,
                out_len=320 * 240, in_len=None, tag=b"fVGT"):
    if in_len is None:
        in_len = fx.chunk_size(index_count, raw_count, palette_count,
                               row_bits, out_len)
    inp = bytearray(fx.IN_CAP)
    inp[0:4] = tag
    struct.pack_into("<I", inp, 4, in_len)
    struct.pack_into("<H", inp, 8, index_count)
    struct.pack_into("<H", inp, 10, raw_count)
    struct.pack_into("<H", inp, 12, palette_count)
    struct.pack_into("<H", inp, 14, row_bits)
    for i in range(16, len(inp)):
        inp[i] = (i * 7 + 3) & 0xFF
    return bytes(inp)


def canvas_bytes(out_len):
    return bytes((i * 13 + 1) & 0xFF for i in range(out_len))


def synthetic_dump(index_count=80, raw_count=0, palette_count=352,
                   row_bits=9, out_len=320 * 240, in_len=None,
                   delta_load=DELTA_LOAD, delta_dump=DELTA_DUMP, heap=HEAP,
                   magic=fx.MAGIC, method=fx.METHOD, tag=b"fVGT",
                   zero_scratch=False, next_ptr=0, output=None,
                   in_cap=fx.IN_CAP):
    data = bytearray(0x600000)
    banner_at = rt.BANNER_LINK + delta_dump
    data[banner_at:banner_at + len(rt.BANNER)] = rt.BANNER
    guest = heap - delta_load
    block_off = guest + delta_dump
    struct.pack_into("<I", data, fx.SCRATCH_LINK + delta_dump,
                     0 if zero_scratch else heap)
    struct.pack_into("<I", data, block_off, next_ptr)
    struct.pack_into("<I", data, block_off + 4, method)
    struct.pack_into("<I", data, block_off + 8, out_len)
    struct.pack_into("<I", data, block_off + 0xC, magic)
    inp = chunk_bytes(index_count=index_count, raw_count=raw_count,
                      palette_count=palette_count, row_bits=row_bits,
                      out_len=out_len, in_len=in_len, tag=tag)
    if output is None:
        output = canvas_bytes(out_len)
    data[block_off + 0x10:block_off + 0x10 + in_cap] = inp
    data[block_off + 0x10 + in_cap:
         block_off + 0x10 + in_cap + len(output)] = output
    return bytes(data)


class TestExtractFrame(unittest.TestCase):
    def test_parses_chunk_and_canvas(self):
        record = fx.extract_frame(synthetic_dump())
        self.assertEqual(record["tag"], b"fVGT")
        self.assertEqual(record["width"], 80)
        self.assertEqual(record["height"], 0)
        self.assertEqual(record["blockcount"], 352)
        self.assertEqual(record["rowcount"], 9)
        self.assertEqual(record["in_len"],
                         fx.chunk_size(80, 0, 352, 9, 320 * 240))
        self.assertEqual(record["out_len"], 320 * 240)
        self.assertEqual(len(record["input"]), fx.IN_CAP)
        self.assertEqual(len(record["output"]), 320 * 240)
        self.assertEqual(record["checks"],
                         {"tag_ok": True, "dims_ok": True,
                          "out_nontrivial": True})
        self.assertEqual(record["delta_load"], DELTA_LOAD)
        self.assertEqual(record["delta_dump"], DELTA_DUMP)

    def test_maps_block_with_arbitrary_deltas(self):
        dump = synthetic_dump(delta_load=0x300000, delta_dump=0x2FF000)
        record = fx.extract_frame(dump, delta_load=0x300000)
        self.assertEqual(record["out_len"], 320 * 240)
        self.assertEqual(record["block_off"],
                         (HEAP - 0x300000) + 0x2FF000)

    def test_chunk_size_matches_live_intro_shapes(self):
        # shapes observed in the VID_INTR.TGV/intro heap scan
        self.assertEqual(fx.chunk_size(1, 0, 0, 1, 320 * 240), 0x270)
        self.assertEqual(fx.chunk_size(1, 43, 20, 6, 320 * 240), 0x1178)
        self.assertEqual(fx.chunk_size(80, 0, 352, 9, 320 * 240), 0x20F4)

    def test_rejects_zero_scratch(self):
        with self.assertRaisesRegex(ValueError, "no fVGT frame"):
            fx.extract_frame(synthetic_dump(zero_scratch=True))

    def test_rejects_bad_magic(self):
        with self.assertRaisesRegex(ValueError, "magic"):
            fx.extract_frame(synthetic_dump(magic=0xDEADBEEF))

    def test_rejects_wrong_tag(self):
        with self.assertRaisesRegex(ValueError, "tag"):
            fx.extract_frame(synthetic_dump(tag=b"kVGT"))

    def test_rejects_dimension_mismatch(self):
        # chunk length declared for a 320x240 canvas but block out_len 0x1000
        with self.assertRaisesRegex(ValueError, "dims_ok"):
            fx.extract_frame(synthetic_dump(
                out_len=0x1000,
                in_len=fx.chunk_size(80, 0, 352, 9, 320 * 240)))

    def test_rejects_zero_index_count(self):
        with self.assertRaisesRegex(ValueError, "dims_ok"):
            fx.extract_frame(synthetic_dump(index_count=0,
                                            in_len=None))

    def test_rejects_out_len_over_cap(self):
        with self.assertRaisesRegex(ValueError, "out_len"):
            fx.extract_frame(synthetic_dump(out_len=fx.OUT_CAP + 16))

    def test_rejects_in_len_over_cap(self):
        with self.assertRaisesRegex(ValueError, "in_len"):
            fx.extract_frame(synthetic_dump(in_len=fx.IN_CAP + 1))

    def test_rejects_trivial_output(self):
        with self.assertRaisesRegex(ValueError, "out_nontrivial"):
            fx.extract_frame(synthetic_dump(
                out_len=0x100, output=bytes(0x100)))

    def test_rejects_chained_block(self):
        with self.assertRaisesRegex(ValueError, "next"):
            fx.extract_frame(synthetic_dump(next_ptr=HEAP - 0x1000))

    def test_requires_banner(self):
        dump = bytearray(synthetic_dump())
        at = rt.BANNER_LINK + DELTA_DUMP
        dump[at:at + len(rt.BANNER)] = b"\x00" * len(rt.BANNER)
        with self.assertRaisesRegex(ValueError, "banner"):
            fx.extract_frame(bytes(dump))


class TestWriteGolden(unittest.TestCase):
    def test_writes_fixture_and_metadata(self):
        record = fx.extract_frame(synthetic_dump())
        with tempfile.TemporaryDirectory() as tmp:
            stem = fx.write_golden(record, tmp, stem="fvgt-01",
                                   provenance={"session": "unit"})
            inp = (Path(tmp) / "fvgt-01.in.bin").read_bytes()
            out = (Path(tmp) / "fvgt-01.out.bin").read_bytes()
            self.assertEqual(inp, record["input"])
            self.assertEqual(out, record["output"])
            meta = json.loads((Path(tmp) / "fvgt-01.json").read_text())
            self.assertEqual(meta["tag"], "fVGT")
            self.assertEqual(meta["width"], 80)
            self.assertEqual(meta["height"], 0)
            self.assertEqual(meta["blockcount"], 352)
            self.assertEqual(meta["rowcount"], 9)
            self.assertEqual(meta["in_len"],
                             fx.chunk_size(80, 0, 352, 9, 320 * 240))
            self.assertEqual(meta["out_len"], 320 * 240)
            self.assertEqual(meta["checks"],
                             {"tag_ok": True, "dims_ok": True,
                              "out_nontrivial": True})
            self.assertEqual(meta["provenance"], {"session": "unit"})
            self.assertEqual(meta["sha256"]["in"],
                             hashlib.sha256(inp).hexdigest())
            self.assertEqual(meta["sha256"]["out"],
                             hashlib.sha256(out).hexdigest())
            self.assertEqual(stem.name, "fvgt-01")


def synthetic_pair_dump(index_count=80, raw_count=0, palette_count=352,
                        row_bits=9, out_len=320 * 240, in_len=None,
                        delta_load=DELTA_LOAD, delta_dump=DELTA_DUMP,
                        heap=HEAP, magic=fx.MAGIC, method=fx.METHOD,
                        tag=b"fVGT", pre=None, output=None, next_ptr=0xDEAD,
                        in_cap=fx.IN_CAP, marker=0x123456):
    data = bytearray(0x600000)
    banner_at = rt.BANNER_LINK + delta_dump
    data[banner_at:banner_at + len(rt.BANNER)] = rt.BANNER
    guest = heap - delta_load
    block_off = guest + delta_dump
    struct.pack_into("<I", data, fx.SCRATCH_LINK + delta_dump, heap)
    struct.pack_into("<I", data, fx.MARKER_LINK + delta_dump, marker)
    struct.pack_into("<I", data, block_off, next_ptr)
    struct.pack_into("<I", data, block_off + 4, method)
    struct.pack_into("<I", data, block_off + 8, out_len)
    struct.pack_into("<I", data, block_off + 0xC, magic)
    inp = chunk_bytes(index_count=index_count, raw_count=raw_count,
                      palette_count=palette_count, row_bits=row_bits,
                      out_len=out_len, in_len=in_len, tag=tag)
    if output is None:
        output = canvas_bytes(out_len)
    if pre is None:
        pre = bytes((i * 29 + 7) & 0xFF for i in range(out_len))
    data[block_off + 0x10:block_off + 0x10 + in_cap] = inp
    base = block_off + 0x10 + in_cap
    data[base:base + len(output)] = output
    data[base + len(output):base + len(output) + len(pre)] = pre
    return bytes(data)


class TestExtractPair(unittest.TestCase):
    def test_parses_post_and_pre(self):
        dump = synthetic_pair_dump()
        record = fx.extract_pair(dump)
        self.assertEqual(record["out_len"], 320 * 240)
        self.assertEqual(len(record["output"]), 320 * 240)
        self.assertEqual(len(record["pre"]), 320 * 240)
        self.assertEqual(record["output"], canvas_bytes(320 * 240))
        self.assertEqual(record["pre"],
                         bytes((i * 29 + 7) & 0xFF
                               for i in range(320 * 240)))
        self.assertEqual(record["checks"],
                         {"tag_ok": True, "dims_ok": True,
                          "out_nontrivial": True, "pre_nontrivial": True,
                          "pair_differs": True})
        self.assertEqual(record["marker"], 0x123456)

    def test_tolerates_garbage_next_pointer(self):
        self.assertEqual(fx.extract_pair(
            synthetic_pair_dump(next_ptr=0xCAFEBABE))["pre"][0:4],
            bytes((i * 29 + 7) & 0xFF for i in range(4)))

    def test_rejects_identical_pre_and_post(self):
        post = canvas_bytes(320 * 240)
        with self.assertRaisesRegex(ValueError, "pair_differs"):
            fx.extract_pair(synthetic_pair_dump(output=post, pre=post))

    def test_rejects_trivial_pre(self):
        with self.assertRaisesRegex(ValueError, "pre_nontrivial"):
            fx.extract_pair(synthetic_pair_dump(
                pre=bytes(320 * 240)))

    def test_rejects_truncated_pair_block(self):
        dump = synthetic_pair_dump()
        scratch = fx.SCRATCH_LINK + DELTA_DUMP
        ptr = struct.unpack_from("<I", dump, scratch)[0]
        block_off = ptr + DELTA_DUMP - DELTA_LOAD
        # keep the input and post but cut the pre tail short
        cut = block_off + 0x10 + fx.IN_CAP + 320 * 240 + 100
        with self.assertRaisesRegex(ValueError, "truncat"):
            fx.extract_pair(dump[:cut])

    def test_rejects_bad_magic(self):
        with self.assertRaisesRegex(ValueError, "magic"):
            fx.extract_pair(synthetic_pair_dump(magic=0xDEADBEEF))

    def test_rejects_wrong_method(self):
        with self.assertRaisesRegex(ValueError, "method"):
            fx.extract_pair(synthetic_pair_dump(method=0x70))

    def test_requires_banner(self):
        dump = bytearray(synthetic_pair_dump())
        at = rt.BANNER_LINK + DELTA_DUMP
        dump[at:at + len(rt.BANNER)] = b"\x00" * len(rt.BANNER)
        with self.assertRaisesRegex(ValueError, "banner"):
            fx.extract_pair(bytes(dump))


class TestWriteGoldenPair(unittest.TestCase):
    def test_writes_pre_fixture_and_metadata(self):
        record = fx.extract_pair(synthetic_pair_dump())
        with tempfile.TemporaryDirectory() as tmp:
            stem = fx.write_golden_pair(
                record, tmp, stem="fvgt-01",
                provenance={"session": "unit"})
            inp = (Path(tmp) / "fvgt-01.in.bin").read_bytes()
            out = (Path(tmp) / "fvgt-01.out.bin").read_bytes()
            pre = (Path(tmp) / "fvgt-01.pre.bin").read_bytes()
            self.assertEqual(inp, record["input"])
            self.assertEqual(out, record["output"])
            self.assertEqual(pre, record["pre"])
            meta = json.loads((Path(tmp) / "fvgt-01.json").read_text())
            self.assertEqual(meta["layout"], "pair")
            self.assertEqual(meta["out_len"], 320 * 240)
            self.assertEqual(meta["pre"]["out_len"], 320 * 240)
            self.assertEqual(meta["pre"]["method"], fx.PRE_METHOD)
            self.assertEqual(meta["sha256"]["pre"],
                             hashlib.sha256(pre).hexdigest())
            self.assertEqual(meta["pre"]["sha256"],
                             hashlib.sha256(pre).hexdigest())
            self.assertTrue(all(meta["checks"].values()))
            self.assertEqual(meta["provenance"]["session"], "unit")
            self.assertIn("pre", meta["provenance"])
            self.assertIn("post", meta["provenance"])
            self.assertEqual(stem.name, "fvgt-01")


class TestGoldenVector(unittest.TestCase):
    def test_committed_fvgt_vector_consistent(self):
        stem = GOLDEN / "fvgt-01"
        if not stem.with_suffix(".json").exists():
            self.skipTest("no committed fvgt vector yet")
        meta = json.loads(stem.with_suffix(".json").read_text())
        inp = stem.with_suffix(".in.bin").read_bytes()
        out = stem.with_suffix(".out.bin").read_bytes()
        self.assertEqual(inp[0:4], b"fVGT")
        self.assertEqual(meta["sha256"]["in"],
                         hashlib.sha256(inp).hexdigest())
        self.assertEqual(meta["sha256"]["out"],
                         hashlib.sha256(out).hexdigest())
        fields = struct.unpack_from("<HHHH", inp, 8)
        self.assertEqual((meta["width"], meta["height"],
                          meta["blockcount"], meta["rowcount"]), fields)
        self.assertEqual(meta["out_len"], len(out))
        self.assertEqual(meta["in_len"], struct.unpack_from("<I", inp, 4)[0])
        self.assertEqual(
            meta["in_len"],
            fx.chunk_size(*fields, meta["out_len"]))
        self.assertTrue(all(meta["checks"].values()))
        if meta.get("layout") == "pair":
            pre = stem.with_suffix(".pre.bin").read_bytes()
            self.assertEqual(len(pre), meta["out_len"])
            self.assertNotEqual(pre, out)
            self.assertEqual(meta["sha256"]["pre"],
                             hashlib.sha256(pre).hexdigest())
            self.assertEqual(meta["pre"]["sha256"],
                             hashlib.sha256(pre).hexdigest())
            self.assertTrue(meta["checks"]["pre_nontrivial"])
            self.assertTrue(meta["checks"]["pair_differs"])


if __name__ == "__main__":
    unittest.main()
