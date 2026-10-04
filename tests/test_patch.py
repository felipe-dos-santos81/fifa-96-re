import contextlib
import io
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

# pre-change byte image of build_cave(TARGET, 7, 1, CAVE), pinned so the
# capture-eax parameter cannot alter the default cave
PRE_CHANGE_CAVE_HEX = (
    "609c06668cd08ec08b5c242c8b6c242883c50289dac1ea1081e3ffff0000be01"
    "00000083ec3089e731c0b90c000000fcf3ab89e789742404896c2408895c2410"
    "8954241466b8000366bb61006631c9cd3183c430079d6183c4045657558b4424"
    "10e92c740300"
)


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

    def test_patch_iso_rejects_nonzero_cave(self):
        # a cave placed on live code would silently overwrite it
        with self.assertRaises(ValueError):
            patch.patch_iso(self.iso, TARGET, TARGET, 1)

    def _assert_changes_only_patched_regions(self, out):
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

    def test_patch_changes_only_target_and_cave(self):
        self._assert_changes_only_patched_regions(
            patch.patch_iso(self.iso, TARGET, CAVE, 1))

    def test_patch_capture_eax_changes_only_target_and_cave(self):
        self._assert_changes_only_patched_regions(
            patch.patch_iso(self.iso, TARGET, CAVE, 1, capture_eax=True))


class TestMalformedIso(unittest.TestCase):
    def test_root_extent_past_eof_raises_value_error(self):
        data = bytearray(17 * 2048)
        pvd = 16 * 2048
        data[pvd + 1:pvd + 6] = b"CD001"
        struct.pack_into("<I", data, pvd + 156 + 2, 0xFFFF)
        struct.pack_into("<I", data, pvd + 156 + 10, 2048)
        with self.assertRaises(ValueError):
            patch.find_iso_file(bytes(data), "FIFA96.EXE")


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

    def test_overwrite_len_rejects_loop_opcodes(self):
        for first in (0xE0, 0xE1, 0xE2, 0xE3):
            image = bytes([first, 0x00]) + b"\x90" * 30
            with self.subTest(opcode=f"{first:#04x}"):
                with self.assertRaises(ValueError):
                    patch.overwrite_len(image, 0)

    def test_overwrite_len_rejects_ret_opcodes(self):
        for vector in (b"\xc2\x00\x00", b"\xc3", b"\xca\x00\x00", b"\xcb"):
            with self.subTest(opcode=f"{vector[0]:#04x}"):
                with self.assertRaises(ValueError):
                    patch.overwrite_len(vector + b"\x90" * 31, 0)

    def test_overwrite_len_rejects_far_branch_opcodes(self):
        far = b"\x00\x00\x00\x00\x00\x00"
        for first in (0x9A, 0xEA):
            with self.subTest(opcode=f"{first:#04x}"):
                with self.assertRaises(ValueError):
                    patch.overwrite_len(
                        bytes([first]) + far + b"\x90" * 25, 0)

    def test_overwrite_len_counts_wrapped_instruction(self):
        # ndisasm prints the first 8 bytes on the instruction line and wraps
        # the rest onto a continuation line; the 10-byte mov must count whole
        image = b"\xc7\x05\x00\x10\x00\x00\x78\x56\x34\x12" + b"\x90" * 22
        self.assertEqual(patch.overwrite_len(image, 0), 10)

    def test_cave_ebp_reports_resume_address(self):
        # the patched `call cave` pushes target+5, so the cave must add
        # overwrite-5 to reach the resume address in the EBP slot
        for capture_eax in (False, True):
            for ow in (7, 5):
                with self.subTest(capture_eax=capture_eax, ow=ow):
                    cave = patch.build_cave(TARGET, ow, 1, CAVE,
                                            capture_eax=capture_eax)
                    i = cave.find(b"\x83\xc5")  # add ebp, imm8
                    self.assertGreaterEqual(
                        i, 0, f"no `add ebp, imm8` in the overwrite={ow} cave")
                    self.assertEqual(cave[i + 2], ow - 5,
                                     f"EBP adjust wrong for overwrite={ow}")

    def test_cave_resets_edi_after_zero_fill(self):
        # rep stosd leaves EDI past the 48-byte frame; INT 31h needs ES:EDI
        # at the structure base, so the cave must reload edi from esp
        cave = patch.build_cave(TARGET, 7, 1, CAVE)
        self.assertIn(b"\xf3\xab\x89\xe7", cave)

    def test_cave_drops_return_address_before_displaced_prologue(self):
        # the call's return address must not remain on the stack when the
        # displaced prologue re-runs, or its [esp+...] loads shift by 4
        cave = patch.build_cave(TARGET, 7, 1, CAVE)
        self.assertIn(b"\x61\x83\xc4\x04", cave)

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

    def test_cave_default_bytes_unchanged(self):
        cave = patch.build_cave(TARGET, 7, 1, CAVE)
        self.assertEqual(cave.hex(), PRE_CHANGE_CAVE_HEX)
        self.assertIn(b"\x8c\xd0", cave)           # mov ax, ss
        self.assertNotIn(b"\x16\x07\xc1\xe0\x10", cave)

    def test_cave_capture_eax_packs_site_in_esi(self):
        cave = patch.build_cave(TARGET, 7, 2, CAVE, capture_eax=True)
        self.assertIn(b"\x16\x07\xc1\xe0\x10", cave)
        self.assertIn(b"\x0d\x02\x00\x00\x00\x89\xc6", cave)  # or/mov esi
        self.assertNotIn(b"\x8c\xd0", cave)        # no mov ax, ss
        self.assertNotIn(b"\xbe", cave)            # ESI is never reloaded

    def test_cave_capture_eax_slots_and_jump(self):
        ow = patch.overwrite_len(self.info["image"], TARGET)
        cave = patch.build_cave(TARGET, ow, 2, CAVE, capture_eax=True)
        self.assertLess(len(cave), 0x103)
        self.assertIn(b"\x89\x74\x24\x04", cave)   # [esp+0x04], esi
        self.assertIn(b"\x89\x6c\x24\x08", cave)   # [esp+0x08], ebp
        self.assertIn(b"\x89\x5c\x24\x10", cave)   # [esp+0x10], ebx
        self.assertIn(b"\x89\x54\x24\x14", cave)   # [esp+0x14], edx
        displaced = self.info["image"][TARGET:TARGET + ow]
        tgt = TARGET + ow
        jmp_off = cave.rindex(b"\xe9")
        rel = struct.unpack_from("<i", cave, jmp_off + 1)[0]
        self.assertEqual(CAVE + jmp_off + 5 + rel, tgt)
        self.assertIn(displaced, cave)


FVGT_RETURN = 0xAE20B
FVGT_RESUME = 0xAE212
# mov eax,[esp+0x14] (4 B) + add esp,0x28 (3 B): the FU-31 brief's
# "overwrite 6, resume 0xAE211" split the add; the verified boundary is 7/212.
FVGT_DISPLACED = bytes.fromhex("8b44241483c428")


class TestFvgtCave(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.iso = ISO.read_bytes()
        lba, size = patch.find_iso_file(cls.iso, "FIFA96.EXE")
        cls.info = le.parse(cls.iso[lba * 2048:lba * 2048 + size])

    def _cave(self):
        return patch.build_fvgt_return_cave(FVGT_RETURN, 7, CAVE + 4, CAVE,
                                            image=self.info["image"])

    def test_overwrite_is_seven_whole_instruction_bytes(self):
        self.assertEqual(
            patch.overwrite_len(self.info["image"], FVGT_RETURN), 7)

    def test_cave_replays_the_displaced_epilogue(self):
        self.assertIn(FVGT_DISPLACED, self._cave())

    def test_cave_jumps_to_verified_resume(self):
        cave = self._cave()
        jmp = cave.rindex(b"\xe9")
        rel = struct.unpack_from("<i", cave, jmp + 1)[0]
        self.assertEqual(CAVE + 4 + jmp + 5 + rel, FVGT_RESUME)

    def test_cave_delta_uses_call_return_not_target(self):
        cave = self._cave()
        self.assertIn(b"\x81\xee" + struct.pack("<I", FVGT_RETURN + 5), cave)
        self.assertNotIn(b"\x81\xee" + struct.pack("<I", FVGT_RETURN), cave)

    def test_cave_reads_ctx_and_chunk_caller_slots(self):
        cave = self._cave()
        self.assertIn(b"\x8b\x5c\x24\x64", cave)   # mov ebx,[esp+0x64] ctx
        self.assertIn(b"\x8b\x6c\x24\x68", cave)   # mov ebp,[esp+0x68] chunk

    def test_cave_reads_canvas_and_checks_tag(self):
        cave = self._cave()
        # decoded surface = ctx[10] (index 10 = byte 0x28) + 0x10 after the
        # entry swap; byte 0x30 is the index array, byte 0x40 the row table
        self.assertIn(b"\x8b\x7b\x28", cave)       # mov edi,[ebx+0x28]
        self.assertIn(b"\x83\xc7\x10", cave)       # add edi,0x10
        self.assertNotIn(b"\x8b\x7b\x30", cave)
        self.assertNotIn(b"\x8b\x7b\x40", cave)
        self.assertIn(b"fVGT", cave)               # tag immediate
        self.assertIn(b"\x66\x56\x47\x54", cave)   # 'FVGT' golden magic
        self.assertIn(b"\x89\x48\x08", cave)       # mov [eax+8], ecx out_len
        # method id is an immediate 0x66, not a register copy
        self.assertIn(b"\xc7\x40\x04\x66\x00\x00\x00", cave)

    def test_cave_gates_on_chunk_fields_not_just_canvas(self):
        # FU-31 skip: chunk width*height == 0 must not capture (the first
        # live fVGT call is a 1x0 init chunk); fields at +8 and +10
        cave = self._cave()
        self.assertIn(b"\x0f\xb7\x55\x08", cave)   # movzx edx,word [ebp+8]
        self.assertIn(b"\x0f\xb7\x45\x0a", cave)   # movzx eax,word [ebp+0xa]
        self.assertIn(b"\x0f\xb7\x0b", cave)       # movzx ecx,word [ebx]
        self.assertIn(b"\x0f\xb7\x53\x04", cave)   # movzx edx,word [ebx+4]

    def test_cave_capture_latest_reuses_the_block(self):
        # errata: instead of first-call copy-once, every qualifying call
        # overwrites the one block; a lost magic/method re-allocates
        cave = self._cave()
        self.assertIn(b"\x81\x78\x0c" + struct.pack("<I", patch.VGT_MAGIC),
                      cave)   # cmp dword [eax+0xc], 'FVGT'
        self.assertIn(b"\x83\x78\x04" + bytes([patch.FVGT_METHOD]),
                      cave)   # cmp dword [eax+4], byte 0x66

    def test_cave_fits_after_scratch_cell(self):
        built = patch.build_fvgt_capture(FVGT_RETURN, CAVE,
                                         image=self.info["image"])
        self.assertEqual(built["scratch_link"], CAVE)
        self.assertEqual(built["return_link"], CAVE + 4)
        self.assertEqual(built["ret_ow"], 7)
        self.assertLessEqual(len(built["blob"]), 0x103)
        self.assertEqual(built["blob"][:4], b"\x00" * 4)

    def test_capture_capacity_enforced(self):
        with self.assertRaises(ValueError):
            patch.build_fvgt_capture(FVGT_RETURN, CAVE,
                                     image=self.info["image"], capacity=0x40)

    def test_patch_iso_fvgt_changes_only_target_and_cave(self):
        out, built = patch.patch_iso_fvgt(self.iso, FVGT_RETURN, CAVE)
        self.assertEqual(len(out), len(self.iso))
        lba, size = patch.find_iso_file(self.iso, "FIFA96.EXE")
        base = lba * 2048
        off_r = patch.link_to_file_offset(self.info, FVGT_RETURN)
        off_c = patch.link_to_file_offset(self.info, CAVE)
        diffs = [i for i in range(len(out)) if out[i] != self.iso[i]]
        self.assertGreater(len(diffs), 5)
        for d in diffs:
            rel = d - base
            self.assertTrue(off_r <= rel < off_r + 5 or
                            off_c <= rel < off_c + len(built["blob"]),
                            f"unexpected diff at file offset 0x{rel:x}")
        call = out[base + off_r:base + off_r + 5]
        self.assertEqual(call[0], 0xE8)
        self.assertEqual(FVGT_RETURN + 5 + struct.unpack("<i", call[1:])[0],
                         CAVE + 4)
        self.assertEqual(bytes(out[base + off_c:base + off_c + 4]), b"\x00" * 4)


FVGT_ENTRY = 0xADEFC
FVGT_ENTRY_RESUME = 0xADF02
FVGT_ENTRY_DISPLACED = bytes.fromhex("56575583ec28")
FVGT_ENTRY_LINK = 0x18A01
FVGT_ENTRY_CAPACITY = 0x2F
FVGT_PAIR_MARKER = 0x18A01


class TestFvgtPairCave(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.iso = ISO.read_bytes()
        lba, size = patch.find_iso_file(cls.iso, "FIFA96.EXE")
        cls.info = le.parse(cls.iso[lba * 2048:lba * 2048 + size])

    def _entry(self):
        return patch.build_fvgt_entry_cave(
            FVGT_ENTRY, 6, FVGT_ENTRY_LINK + 4, FVGT_PAIR_MARKER,
            image=self.info["image"])

    def _pair(self):
        return patch.build_fvgt_pair_cave(
            FVGT_RETURN, 7, CAVE + 4, CAVE, FVGT_PAIR_MARKER,
            image=self.info["image"])

    def test_entry_overwrite_is_six_whole_instruction_bytes(self):
        self.assertEqual(
            patch.overwrite_len(self.info["image"], FVGT_ENTRY), 6)

    def test_entry_cave_replays_the_displaced_prologue(self):
        self.assertIn(FVGT_ENTRY_DISPLACED, self._entry())

    def test_entry_cave_jumps_to_verified_resume(self):
        cave = self._entry()
        jmp = cave.rindex(b"\xe9")
        rel = struct.unpack_from("<i", cave, jmp + 1)[0]
        self.assertEqual(FVGT_ENTRY_LINK + 4 + jmp + 5 + rel,
                         FVGT_ENTRY_RESUME)

    def test_entry_cave_records_pre_pointer_from_ctx10(self):
        cave = self._entry()
        # ctx is cdecl arg 1 at [esp+8] at hook entry; one push shifts it
        self.assertIn(b"\x8b\x44\x24\x0c", cave)   # mov eax,[esp+0xc]
        self.assertIn(b"\x8b\x40\x28", cave)       # mov eax,[eax+0x28]
        self.assertIn(b"\x89\x86" + struct.pack("<I", FVGT_PAIR_MARKER),
                      cave)                        # mov [esi+marker],eax
        self.assertIn(b"\x8b\x74\x24\x04", cave)   # mov esi,[esp+4]

    def test_entry_cave_fits_the_only_spare_run(self):
        self.assertLessEqual(len(self._entry()), FVGT_ENTRY_CAPACITY - 4)

    def test_pair_cave_replays_the_displaced_epilogue(self):
        self.assertIn(FVGT_DISPLACED, self._pair())

    def test_pair_cave_jumps_to_verified_resume(self):
        cave = self._pair()
        jmp = cave.rindex(b"\xe9")
        rel = struct.unpack_from("<i", cave, jmp + 1)[0]
        self.assertEqual(CAVE + 4 + jmp + 5 + rel, FVGT_RESUME)

    def test_pair_cave_reads_post_from_ctx10_pre_from_marker(self):
        cave = self._pair()
        self.assertIn(b"\x8b\x7b\x28", cave)       # mov edi,[ebx+0x28]
        self.assertIn(b"\x8b\x9e" + struct.pack("<I", FVGT_PAIR_MARKER),
                      cave)                        # mov ebx,[esi+marker]
        self.assertIn(b"\x83\xc7\x10", cave)       # add edi,0x10
        self.assertIn(b"\x83\xc3\x10", cave)       # add ebx,0x10

    def test_pair_cave_allocates_input_plus_two_surfaces(self):
        cave = self._pair()
        size = 0x10 + patch.FVGT_IN_CAP + 2 * patch.FVGT_OUT_CAP
        self.assertIn(b"\x68" + struct.pack("<I", size), cave)
        self.assertIn(b"\xc6\x40\x04" + bytes([patch.FVGT_METHOD]), cave)
        self.assertIn(b"\xc7\x40\x0c" + struct.pack("<I", patch.VGT_MAGIC),
                      cave)

    def test_pair_cave_preserves_pre_pointer_across_allocator(self):
        # FUN_00098bf8 preserves only ESI/EDI; the pre surface lives in EBX
        # and must be spilled around the call
        cave = self._pair()
        size = struct.pack(
            "<I", 0x10 + patch.FVGT_IN_CAP + 2 * patch.FVGT_OUT_CAP)
        self.assertIn(b"\x53\x51\x6a\x00\x68" + size, cave)
        call = cave.index(b"\xe8", cave.index(b"\x68" + size))
        self.assertEqual(cave[call + 5:call + 10],
                         b"\x83\xc4\x0c\x59\x5b")

    def test_pair_cave_gates_on_chunk_and_dims(self):
        cave = self._pair()
        self.assertIn(b"fVGT", cave)
        self.assertIn(b"\x0f\xb7\x55\x08", cave)
        self.assertIn(b"\x0f\xb7\x45\x0a", cave)
        self.assertIn(b"\x0f\xb7\x0b", cave)
        self.assertIn(b"\x0f\xb7\x53\x04", cave)

    def test_pair_capture_placement_and_zero_cells(self):
        built = patch.build_fvgt_pair_capture(
            FVGT_ENTRY, FVGT_RETURN, FVGT_ENTRY_LINK, CAVE,
            image=self.info["image"])
        self.assertEqual(built["marker_link"], FVGT_ENTRY_LINK)
        self.assertEqual(built["entry_link"], FVGT_ENTRY_LINK + 4)
        self.assertEqual(built["scratch_link"], CAVE)
        self.assertEqual(built["return_link"], CAVE + 4)
        self.assertEqual(built["entry_ow"], 6)
        self.assertEqual(built["ret_ow"], 7)
        self.assertEqual(built["entry_blob"][:4], b"\x00" * 4)
        self.assertEqual(built["ret_blob"][:4], b"\x00" * 4)
        self.assertLessEqual(len(built["entry_blob"]), FVGT_ENTRY_CAPACITY)
        self.assertLessEqual(len(built["ret_blob"]), 0x103)

    def test_pair_capture_entry_capacity_enforced(self):
        with self.assertRaises(ValueError):
            patch.build_fvgt_pair_capture(
                FVGT_ENTRY, FVGT_RETURN, FVGT_ENTRY_LINK, CAVE,
                image=self.info["image"], entry_capacity=0x10)

    def test_pair_capture_return_capacity_enforced(self):
        with self.assertRaises(ValueError):
            patch.build_fvgt_pair_capture(
                FVGT_ENTRY, FVGT_RETURN, FVGT_ENTRY_LINK, CAVE,
                image=self.info["image"], return_capacity=0x40)

    def test_patch_iso_fvgt_pair_changes_only_four_regions(self):
        out, built = patch.patch_iso_fvgt_pair(
            self.iso, FVGT_ENTRY, FVGT_RETURN, FVGT_ENTRY_LINK, CAVE)
        self.assertEqual(len(out), len(self.iso))
        lba, size = patch.find_iso_file(self.iso, "FIFA96.EXE")
        base = lba * 2048
        regions = []
        for target, link in ((FVGT_ENTRY, built["entry_link"]),
                             (FVGT_RETURN, built["return_link"])):
            off_t = patch.link_to_file_offset(self.info, target)
            off_c = patch.link_to_file_offset(self.info, link - 4)
            blob = built["entry_blob"] if target == FVGT_ENTRY \
                else built["ret_blob"]
            regions.append((off_t, off_t + 5))
            regions.append((off_c, off_c + len(blob)))
        diffs = [i for i in range(len(out)) if out[i] != self.iso[i]]
        self.assertGreater(len(diffs), 5)
        for d in diffs:
            rel = d - base
            self.assertTrue(any(a <= rel < b for a, b in regions),
                            f"unexpected diff at file offset 0x{rel:x}")
        off_e = patch.link_to_file_offset(self.info, FVGT_ENTRY)
        off_r = patch.link_to_file_offset(self.info, FVGT_RETURN)
        for off, link in ((off_e, built["entry_link"]),
                          (off_r, built["return_link"])):
            call = out[base + off:base + off + 5]
            self.assertEqual(call[0], 0xE8)
            target = FVGT_ENTRY if link == built["entry_link"] \
                else FVGT_RETURN
            self.assertEqual(target + 5 + struct.unpack("<i", call[1:])[0],
                             link)


class TestCli(unittest.TestCase):
    def test_print_overwrite(self):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = patch.main(["--iso", str(ISO), "--target", "0x9E718",
                             "--print-overwrite"])
        self.assertEqual(rc, 0)
        self.assertEqual(buf.getvalue().strip(), "7")

    def test_fvgt_and_vgt_capture_are_mutually_exclusive(self):
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            with self.assertRaises(SystemExit):
                patch.main(["--iso", str(ISO), "--target", "0xAE20B",
                            "--vgt-capture", "--fvgt-capture",
                            "--cave", "0x6728D", "--site-id", "1"])

    def test_fvgt_pair_is_mutually_exclusive_with_the_other_modes(self):
        for other in ("--fvgt-capture", "--vgt-capture"):
            with self.subTest(other=other):
                err = io.StringIO()
                with contextlib.redirect_stderr(err):
                    with self.assertRaises(SystemExit):
                        patch.main(["--iso", str(ISO), "--target", "0xADEFC",
                                    "--return-target", "0xAE20B",
                                    "--fvgt-pair", other,
                                    "--entry-cave", "0x18A01",
                                    "--cave", "0x6728D", "--site-id", "1"])


if __name__ == "__main__":
    unittest.main()
