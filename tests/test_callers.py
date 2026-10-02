import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fifa96_callers as callers  # noqa: E402

ISO = ROOT / "game" / "FIFAPCCD96.iso"

DECOMPRESSOR = 0x9E718
DECOMPRESSOR_SITES = [0x9E86C, 0x9E884, 0xCAE32]
WRAPPER = 0x9E860
WRAPPER_SITES = [
    0x14C65, 0x14D76, 0x18CE8, 0x23C0C, 0x24DE6, 0x26E47,
    0x4A32A, 0x4A376, 0x4A42B, 0x4A5DC, 0x4B000, 0x78E5E,
    0x78EAB, 0x78EB7, 0xAE474,
]


class TestDirectCallers(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.image = callers.load_image(str(ISO))

    def test_decompressor_sites(self):
        got = callers.direct_callers(self.image, DECOMPRESSOR)
        self.assertEqual([c["opcode"] for c in got], DECOMPRESSOR_SITES)
        self.assertEqual([c["ret"] for c in got],
                         [site + 5 for site in DECOMPRESSOR_SITES])

    def test_wrapper_sites(self):
        got = callers.direct_callers(self.image, WRAPPER)
        self.assertEqual(sorted(c["opcode"] for c in got),
                         sorted(WRAPPER_SITES))

    def test_no_callers_for_unused_address(self):
        self.assertEqual(callers.direct_callers(self.image, 0x123456), [])

    def test_ret_is_always_opcode_plus_five(self):
        for c in callers.direct_callers(self.image, WRAPPER):
            self.assertEqual(c["ret"], c["opcode"] + 5)


if __name__ == "__main__":
    unittest.main()
