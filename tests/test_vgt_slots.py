import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fifa96_probe as probe  # noqa: E402
import fifa96_runtime as rt  # noqa: E402
import fifa96_vgt_capture as vgt  # noqa: E402

DELTA_LOAD = 0x1FC000
DELTA_DUMP = 0x1FC010
SCRATCH = 0x400000
HEAP = 0x410000
STRIDE = 0x10 + vgt.IN_CAP + vgt.OUT_CAP


def tprobe(method=0x10, site=1, target_ret=0x29A71F):
    """FU-21 descriptor frame: caller_lo carries the observed method byte."""
    payload = struct.pack("<IIII", site, method & 0xFF, 0, target_ret)
    return bytes([probe.T_PROBE]) + struct.pack("<HH", 0, 16) + payload


def block_record(method, out_len, order, header24=None):
    if header24 is None:
        header24 = out_len
    inp = bytearray(vgt.IN_CAP)
    inp[0] = method
    inp[1] = 0xFB
    inp[2] = (header24 >> 16) & 0xFF
    inp[3] = (header24 >> 8) & 0xFF
    inp[4] = header24 & 0xFF
    for i in range(5, len(inp)):
        inp[i] = (i * 7 + order) & 0xFF
    out = bytes((i * 13 + 1 + order) & 0xFF for i in range(out_len))
    return bytes(inp), out


def dump_off(guest, delta_load=DELTA_LOAD, delta_dump=DELTA_DUMP):
    return guest + delta_dump - delta_load


def new_block(data, addr, next_ptr, method, out_len, magic=vgt.VGT_MAGIC,
              header24=None):
    struct.pack_into("<I", data, dump_off(addr), next_ptr)
    struct.pack_into("<I", data, dump_off(addr + 4), method)
    struct.pack_into("<I", data, dump_off(addr + 8), out_len)
    struct.pack_into("<I", data, dump_off(addr + 0xC), magic)
    inp, out = block_record(method, out_len, addr & 0xFF, header24=header24)
    data[dump_off(addr + 0x10):dump_off(addr + 0x10) + len(inp)] = inp
    data[dump_off(addr + 0x10 + vgt.IN_CAP):
         dump_off(addr + 0x10 + vgt.IN_CAP) + len(out)] = out


def synthetic_dump(records, scratch=SCRATCH, delta_load=DELTA_LOAD,
                   delta_dump=DELTA_DUMP, magic=vgt.VGT_MAGIC):
    """Linked list from `records` in capture order (oldest first)."""
    data = bytearray(0x500000)
    banner_at = rt.BANNER_LINK + delta_dump
    data[banner_at:banner_at + len(rt.BANNER)] = rt.BANNER
    head = 0
    for i, rec in enumerate(records):
        method, out_len = rec[0], rec[1]
        header24 = rec[2] if len(rec) > 2 else None
        addr = HEAP + i * STRIDE
        new_block(data, addr, head, method, out_len, magic=magic,
                  header24=header24)
        head = addr
    struct.pack_into("<I", data, vgt.SCRATCH_LINK + delta_dump, head)
    return bytes(data)


def legacy_dump(method=0x10, out_len=0x100, scratch=SCRATCH):
    """FU-20 single-block layout: magic at +0, no next pointer."""
    data = bytearray(0x500000)
    banner_at = rt.BANNER_LINK + DELTA_DUMP
    data[banner_at:banner_at + len(rt.BANNER)] = rt.BANNER
    struct.pack_into("<I", data, vgt.SCRATCH_LINK + DELTA_DUMP, HEAP)
    header = struct.pack("<I", vgt.VGT_MAGIC) + \
        bytes([method, 0xFB, 0, 0]) + struct.pack("<I", out_len)
    data[dump_off(HEAP):dump_off(HEAP) + 0x10] = header
    inp, out = block_record(method, out_len, 0)
    data[dump_off(HEAP + 0x10):dump_off(HEAP + 0x10) + len(inp)] = inp
    data[dump_off(HEAP + 0x10 + vgt.IN_CAP):
         dump_off(HEAP + 0x10 + vgt.IN_CAP) + len(out)] = out
    return bytes(data)


class TestExtractSlots(unittest.TestCase):
    def test_walks_slots_in_capture_order(self):
        dump = synthetic_dump([(0x10, 0x100), (0x16, 0x80), (0x6A, 0x20)])
        records = vgt.extract_records(dump, tprobe(0x10))
        self.assertEqual([r["method"] for r in records], [0x10, 0x16, 0x6A])
        for i, rec in enumerate(records):
            self.assertNotIn(False, rec["checks"].values())
            self.assertEqual(rec["header24"], rec["out_len"])
            self.assertEqual(rec["block_off"],
                             dump_off(HEAP + i * STRIDE))
            self.assertEqual(rec["delta_load"], DELTA_LOAD)

    def test_rejects_duplicate_method(self):
        dump = synthetic_dump([(0x16, 0x100), (0x16, 0x80)])
        with self.assertRaisesRegex(ValueError, "duplicate"):
            vgt.extract_records(dump, tprobe(0x16))

    def test_method_filter_selects_slot(self):
        dump = synthetic_dump([(0x10, 0x100), (0x16, 0x80)])
        records = vgt.extract_records(dump, tprobe(0x10), method={0x16})
        self.assertEqual([r["method"] for r in records], [0x16])
        with self.assertRaisesRegex(ValueError, "not captured"):
            vgt.extract_records(dump, tprobe(0x10), method={0x32})

    def test_legacy_single_block_still_parses(self):
        records = vgt.extract_records(legacy_dump(), tprobe(0x10))
        self.assertEqual([r["method"] for r in records], [0x10])
        self.assertEqual(records[0]["out_len"], 0x100)

    def test_observed_methods_from_descriptor_frames(self):
        trace = tprobe(0x10) + tprobe(0x32) + tprobe(0x10) + tprobe(0x6A)
        self.assertEqual(vgt.observed_methods(trace), [0x10, 0x32, 0x6A])

    def test_requires_live_descriptor_frames(self):
        dump = synthetic_dump([(0x10, 0x100)])
        with self.assertRaisesRegex(ValueError, "descriptor frames"):
            vgt.extract_records(dump, tprobe(0x10, site=2))

    def test_rejects_cycle(self):
        data = bytearray(synthetic_dump([(0x10, 0x100), (0x16, 0x80)]))
        struct.pack_into("<I", data, dump_off(HEAP), HEAP + STRIDE)
        struct.pack_into("<I", data, dump_off(HEAP + STRIDE), HEAP)
        with self.assertRaisesRegex(ValueError, "cycle"):
            vgt.extract_records(bytes(data), tprobe(0x10))

    def test_rejects_bad_magic(self):
        with self.assertRaisesRegex(ValueError, "magic"):
            vgt.extract_records(synthetic_dump([(0x10, 0x100)],
                                               magic=0xDEADBEEF),
                                tprobe(0x10))

    def test_rejects_out_len_over_cap(self):
        with self.assertRaisesRegex(ValueError, "over cap"):
            vgt.extract_records(synthetic_dump([(0x10, vgt.OUT_CAP + 1)]),
                                tprobe(0x10))

    def test_single_record_compat_wrapper(self):
        record = vgt.extract(synthetic_dump([(0x10, 0x100)]), tprobe(0x10))
        self.assertEqual(record["method"], 0x10)
        with self.assertRaisesRegex(ValueError, "pass method"):
            vgt.extract(synthetic_dump([(0x10, 0x100), (0x16, 0x80)]),
                        tprobe(0x10))

    def test_selector_set_matches_dispatch_tree(self):
        # 0x30 (huff) is a valid selector; 0x70 is rejected by the tree.
        self.assertIn(0x30, vgt.KNOWN_METHODS)
        self.assertNotIn(0x70, vgt.KNOWN_METHODS)
        self.assertIn(0x6E, vgt.KNOWN_METHODS)
        self.assertEqual(vgt.LENGTH_CONTRACT_METHODS, {0x10, 0x6A, 0x6E})

    def test_noncontract_header24_is_informational(self):
        # huff (raw 0x31 -> selector 0x30) does not carry the decoded
        # length in the BE24 header field.
        dump = synthetic_dump([(0x31, 0x80, 0x1234)])
        records = vgt.extract_records(dump, tprobe(0x31))
        self.assertEqual(records[0]["method"], 0x31)
        self.assertEqual(records[0]["checks"],
                         {"signature": True, "method_known": True,
                          "output_nontrivial": True})

    def test_contract_method_still_enforces_length(self):
        dump = synthetic_dump([(0x10, 0x80, 0x99)])
        with self.assertRaisesRegex(ValueError, "length_matches"):
            vgt.extract_records(dump, tprobe(0x10))

    def test_filter_ignores_bad_unrequested_slot(self):
        dump = synthetic_dump([(0x10, 0x80, 0x99), (0x46, 0x40)])
        with self.assertRaisesRegex(ValueError, "length_matches"):
            vgt.extract_records(dump, tprobe(0x10))
        records = vgt.extract_records(dump, tprobe(0x10), method={0x46})
        self.assertEqual([r["method"] for r in records], [0x46])

    def test_filter_accepts_selector_for_raw_method(self):
        dump = synthetic_dump([(0x31, 0x80, 0x1234)])
        records = vgt.extract_records(dump, tprobe(0x31), method={0x30})
        self.assertEqual([r["method"] for r in records], [0x31])

    def test_write_golden_per_method(self):
        import tempfile
        records = vgt.extract_records(
            synthetic_dump([(0x10, 0x100), (0x16, 0x80)]), tprobe(0x10))
        with tempfile.TemporaryDirectory() as tmp:
            stems = [vgt.write_golden(r, tmp) for r in records]
            self.assertEqual([s.name for s in stems],
                             ["record-10", "record-16"])
            self.assertTrue((Path(tmp) / "record-16.json").exists())


if __name__ == "__main__":
    unittest.main()
