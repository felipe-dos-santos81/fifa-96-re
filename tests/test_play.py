import hashlib
import re
import struct
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "build" / "fifa96_play"
GOLDEN = ROOT / "tests" / "golden"
VGT = GOLDEN / "vgt"
EACS = GOLDEN / "eacs"
PLAY = ROOT / "build" / "play"
ISO = ROOT / "game" / "FIFAPCCD96.iso"
FRAME_RE = re.compile(r"^frame (\d+): (\d+)x(\d+) (ppm|modex) sha256=([0-9a-f]{64})$")
AUDIO_RE = re.compile(r"^audio: frames=(\d+) rate=(\d+) channels=(\d+) sha256=([0-9a-f]{64})$")
SPRITE_RE = re.compile(
    r"^sprite: entries=(\d+) entry=(\d+) name=(\S+) frames=(\d+) frame=(\d+) "
    r"(\d+)x(\d+) (ppm) sha256=([0-9a-f]{64})$", re.M)
PPM_RE = re.compile(rb"^P6\n(\d+) (\d+)\n255\n")
AUDIO_SHA = {
    "viv0": "55b8e1b0ea10cfd62db323519f94e8a1af548acd52dc5d62862dbb028b14f3e0",
    "bnk1": "5560690cd27562849ac26618043249cde153d522f3ff2b579f6585952c2272f1",
}
ISO_COUNTS = {
    "video": 73,
    "audio": 513,
    "crd": 1,
    "sprite": 10,
}
ISO_VIDEO0_SHA = "1f7d01c1fa2bda9593c04e974f95038fcc63103b2cb258bbd44109003d2aaa1f"
ISO_AUDIO0_SHA = "4840a05a9d6c4c3a138d1b55b2e4c1315950c3c783b8ac83da8666663754e034"
ISO_SPRITE0_SHA = "0732bf3b40ad9272153929b0e58c3eb1a2164c01d683ab8c0d033489fde9e7d9"


def run(*args):
    return subprocess.run([str(TOOL), *args], cwd=ROOT, capture_output=True, text=True)


def build_video_stream(path):
    pre = (VGT / "fvgt-01.pre.bin").read_bytes()
    inp = (VGT / "fvgt-01.in.bin").read_bytes()
    key = (VGT / "kvgt-frame-01.bin").read_bytes()
    palette = key[0x14:0x14 + 768]
    lit = b"\x6a\xfb" + len(pre).to_bytes(3, "big") + pre
    kvgt = (b"kVGT" + struct.pack("<I", 0x14 + len(palette) + len(lit)) +
            struct.pack("<HHHH", 320, 240, 0, 256) + b"\0\0\0\0" + palette + lit)
    chunk = inp[:struct.unpack_from("<I", inp, 4)[0]]
    path.write_bytes(kvgt + chunk)
    return pre, (VGT / "fvgt-01.out.bin").read_bytes(), palette


def ppm(pixels, palette, width, height):
    out = bytearray(b"P6\n%d %d\n255\n" % (width, height))
    for i in range(width * height):
        out += palette[pixels[i] * 3:pixels[i] * 3 + 3]
    return bytes(out)


def modex(pixels, width, height):
    planes = [bytearray(height * 80) for _ in range(4)]
    for plane in range(4):
        count = width - plane
        if count < 4:
            continue
        for row in range(height):
            for k in range(count // 4):
                planes[plane][row * 80 + k] = pixels[row * width + k * 4 + plane]
    return b"".join(bytes(p) for p in planes)


def wav(pcm, rate, channels):
    byte_rate = rate * channels * 2
    hdr = (b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVEfmt " +
           struct.pack("<IHHIIHH", 16, 1, channels, rate, byte_rate,
                       channels * 2, 16) + b"data" + struct.pack("<I", len(pcm)))
    return hdr + pcm


def wav_fields(path):
    b = path.read_bytes()
    assert b[:4] == b"RIFF" and b[8:12] == b"WAVE" and b[12:16] == b"fmt "
    fmt, channels, rate, _, _, bits = struct.unpack_from("<HHIIHH", b, 20)
    data_tag = 12 + 8 + struct.unpack_from("<I", b, 16)[0]
    assert fmt == 1 and bits == 16
    assert b[data_tag:data_tag + 4] == b"data"
    size = struct.unpack_from("<I", b, data_tag + 4)[0]
    assert size == len(b) - data_tag - 8
    return rate, channels, size // (channels * 2), b


def ppm_info(blob):
    m = PPM_RE.match(blob)
    assert m, blob[:16]
    return int(m.group(1)), int(m.group(2)), m.end()


GRAY = bytes(i for i in range(256) for _ in range(3))
PIX0 = bytes(range(12))
PIX1 = bytes([200, 201, 202, 203])
PIX2 = bytes([7])


def sprite_frame(pixels, width, height, second=0, pivot=(0, 0), word12=0):
    return (bytes([0x7B]) + second.to_bytes(3, "little") +
            struct.pack("<HHHH", width, height, pivot[0], pivot[1]) +
            struct.pack("<I", word12) + pixels)


def shpi(entries):
    count = len(entries)
    off = 16 + 8 * count
    table = bytearray()
    blobs = bytearray()
    for name, blob in entries:
        table += name.encode() + struct.pack("<I", off)
        blobs += blob
        off += len(blob)
    return (b"SHPI" + struct.pack("<II", off, count) + b"GIMX" +
            bytes(table) + bytes(blobs))


def bigf(records):
    count = len(records)
    table_end = 0x10
    for name, _ in records:
        table_end += 8 + len(name) + 1
    off = table_end
    table = bytearray()
    data = bytearray()
    for name, blob in records:
        table += struct.pack(">II", off, len(blob)) + name.encode() + b"\0"
        data += blob
        off += len(blob)
    return (b"BIGF" + struct.pack(">III", table_end + len(data), count, table_end) +
            bytes(table) + bytes(data))


def build_sprite_bigf(path):
    bank0 = shpi([("f000", sprite_frame(PIX0, 4, 3, pivot=(2, 1))),
                  ("f001", sprite_frame(PIX1, 2, 2))])
    bank1 = shpi([("s000", sprite_frame(PIX2, 1, 1, word12=0x11223344))])
    path.write_bytes(bigf([("raw.fsh", bank0), ("one.fsh", bank1)]))


PAL6 = bytes([1, 32, 63] + [(i * 3) & 63 for i in range(765)])
PAL8 = bytes((v * 255) // 63 for v in PAL6)
CHUNK_PAL = bytes.fromhex("22000000000101000001000000000000") + PAL6
CHUNK_24 = bytes.fromhex("7c000000020000007b000000b4ffffff1000000000000000")


def build_palette_sprite(path):
    pal_bank = shpi([("p000", sprite_frame(b"\x00", 1, 1, second=20) + b"\x00\x00\x00" +
                      CHUNK_PAL)])
    player_bank = shpi([("c000", sprite_frame(b"\x05", 1, 1, second=20) + b"\x00\x00\x00" +
                         CHUNK_24)])
    path.write_bytes(bigf([("pal.fsh", pal_bank), ("play.fsh", player_bank)]))


class TestHelp(unittest.TestCase):
    def test_help_lists_modes(self):
        r = run("--help")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("video", r.stdout)
        self.assertIn("audio", r.stdout)
        self.assertIn("sprite", r.stdout)


class TestVideo(unittest.TestCase):
    def setUp(self):
        PLAY.mkdir(parents=True, exist_ok=True)
        self.stream = PLAY / "test-video.tgv"
        self.pre, self.post, self.palette = build_video_stream(self.stream)

    def test_sequence_writes_expected_ppm(self):
        out = PLAY / "seq"
        r = run("video", str(self.stream), "--out", str(out), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        want0 = ppm(self.pre, self.palette, 320, 240)
        want1 = ppm(self.post, self.palette, 320, 240)
        self.assertEqual((out / "frame-0000.ppm").read_bytes(), want0)
        self.assertEqual((out / "frame-0001.ppm").read_bytes(), want1)
        lines = [FRAME_RE.match(x) for x in r.stdout.splitlines() if x.startswith("frame")]
        self.assertEqual([(m.group(1), m.group(2), m.group(3), m.group(4)) for m in lines],
                         [("0", "320", "240", "ppm"), ("1", "320", "240", "ppm")])
        self.assertEqual([m.group(5) for m in lines],
                         [hashlib.sha256(want0).hexdigest(), hashlib.sha256(want1).hexdigest()])

    def test_frame_selects_single_file(self):
        single = PLAY / "one.ppm"
        r = run("video", str(self.stream), "--frame", "1", "--out", str(single),
                "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(single.read_bytes(), ppm(self.post, self.palette, 320, 240))
        self.assertIn("frame 1: 320x240 ppm sha256=", r.stdout)

    def test_modex_writes_planar_frame(self):
        out = PLAY / "frame.modex"
        r = run("video", str(self.stream), "--frame", "1", "--modex",
                "--out", str(out), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        want = modex(self.post, 320, 240)
        self.assertEqual(out.read_bytes(), want)
        m = [FRAME_RE.match(x) for x in r.stdout.splitlines() if x.startswith("frame")][0]
        self.assertEqual((m.group(1), m.group(4), m.group(5)),
                         ("1", "modex", hashlib.sha256(want).hexdigest()))

    def test_max_frames_limits_sequence(self):
        out = PLAY / "max"
        r = run("video", str(self.stream), "--max-frames", "1", "--out", str(out),
                "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(sorted(p.name for p in out.iterdir()), ["frame-0000.ppm"])
        self.assertEqual(len([x for x in r.stdout.splitlines() if x.startswith("frame")]), 1)

    def test_malformed_stream_fails_loudly(self):
        bad = PLAY / "bad.tgv"
        bad.write_bytes(b"kVGT" + struct.pack("<I", 4) + b"\0\0\0\0")
        r = run("video", str(bad), "--out", str(PLAY / "bad"))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("error", r.stderr.lower())

    def test_missing_file_fails(self):
        r = run("video", str(PLAY / "nope.tgv"))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("error", r.stderr.lower())


class TestAudioRaw(unittest.TestCase):
    def test_bank_h_pcm16_stereo_matches_fixture_bytes(self):
        raw = (EACS / "bank-h.eacs").read_bytes()
        rate = struct.unpack_from("<I", raw, 4)[0]
        data_off = struct.unpack_from("<I", raw, 0x18)[0] or 0x20
        data = raw[data_off:]
        out = PLAY / "bank-h.wav"
        r = run("audio", str(EACS / "bank-h.eacs"), "--out", str(out), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        want = wav(data, rate, 2)
        self.assertEqual(out.read_bytes(), want)
        self.assertEqual(hashlib.sha256(out.read_bytes()).hexdigest(),
                         hashlib.sha256(want).hexdigest())
        m = AUDIO_RE.match(r.stdout.strip())
        self.assertIsNotNone(m, r.stdout)
        self.assertEqual((m.group(2), m.group(3)), (str(rate), "2"))
        self.assertEqual(int(m.group(1)), len(data) // 4)
        self.assertEqual(m.group(4), hashlib.sha256(want).hexdigest())

    def test_truncated_input_fails(self):
        bad = PLAY / "bad.eacs"
        bad.write_bytes(b"EACS" + b"\0" * 12)
        r = run("audio", str(bad))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("error", r.stderr.lower())


class TestAudioContainers(unittest.TestCase):
    def test_viv_index_decodes_delta_mono(self):
        raw = (EACS / "bank-t019.viv").read_bytes()
        rec_off = struct.unpack_from(">I", raw, 0x10)[0]
        rate = struct.unpack_from("<I", raw, rec_off + 4)[0]
        units = struct.unpack_from("<I", raw, rec_off + 0x0C)[0]
        out = PLAY / "viv0.wav"
        r = run("audio", "--viv", str(EACS / "bank-t019.viv"), "--index", "0",
                "--out", str(out), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        got_rate, got_ch, frames, blob = wav_fields(out)
        self.assertEqual((got_rate, got_ch, frames), (rate, 1, units))
        samples = struct.unpack_from("<%dh" % units, blob, 44)
        self.assertEqual(samples[:8], (11, 41, 104, 240, 533, 912, 963, 917))
        self.assertEqual(samples[-1], -95)
        self.assertEqual(hashlib.sha256(blob).hexdigest(), AUDIO_SHA["viv0"])
        m = AUDIO_RE.match(r.stdout.strip())
        self.assertEqual((m.group(1), m.group(2), m.group(3), m.group(4)),
                         (str(units), str(rate), "1", hashlib.sha256(blob).hexdigest()))

    def test_bnk_id_decodes_delta(self):
        raw = (GOLDEN / "sfx_game.bnk").read_bytes()
        desc = struct.unpack_from("<I", raw, 4)[0]
        eacs = struct.unpack_from("<I", raw, desc + 4)[0]
        rate = struct.unpack_from("<I", raw, eacs + 4)[0]
        units = struct.unpack_from("<I", raw, eacs + 0x0C)[0]
        channels = 1 if raw[eacs + 9] == 1 else 2
        out = PLAY / "bnk1.wav"
        r = run("audio", "--bnk", str(GOLDEN / "sfx_game.bnk"), "--id", "1",
                "--out", str(out), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        got_rate, got_ch, frames, blob = wav_fields(out)
        self.assertEqual((got_rate, got_ch, frames), (rate, channels, units))
        samples = struct.unpack_from("<%dh" % (units * channels), blob, 44)
        self.assertEqual(samples[:8], (11, -19, 44, 35, 43, 80, 61, 92))
        self.assertEqual(samples[-1], -791)
        self.assertEqual(hashlib.sha256(blob).hexdigest(), AUDIO_SHA["bnk1"])
        m = AUDIO_RE.match(r.stdout.strip())
        self.assertEqual((m.group(1), m.group(2), m.group(3), m.group(4)),
                         (str(units), str(rate), str(channels),
                          hashlib.sha256(blob).hexdigest()))

    def test_viv_name_selects_same_record(self):
        by_name = PLAY / "viv-name.wav"
        by_index = PLAY / "viv-index.wav"
        r = run("audio", "--viv", str(EACS / "bank-t019.viv"), "--name", "tmt01901.spc",
                "--out", str(by_name), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        r = run("audio", "--viv", str(EACS / "bank-t019.viv"), "--index", "0",
                "--out", str(by_index), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(by_name.read_bytes(), by_index.read_bytes())


class TestSpriteSynthetic(unittest.TestCase):
    def setUp(self):
        PLAY.mkdir(parents=True, exist_ok=True)
        self.fixture = PLAY / "sprite-bigf.pvi"
        build_sprite_bigf(self.fixture)

    def test_grayscale_ppm_exact(self):
        out = PLAY / "sprite-grey.ppm"
        r = run("sprite", str(self.fixture), "--frame", "0", "--out", str(out),
                "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        want = ppm(PIX0, GRAY, 4, 3)
        self.assertEqual(out.read_bytes(), want)
        m = SPRITE_RE.match(r.stdout.strip())
        self.assertIsNotNone(m, r.stdout)
        self.assertEqual(m.groups()[:8],
                         ("2", "0", "raw.fsh", "2", "0", "4", "3", "ppm"))
        self.assertEqual(m.group(9), hashlib.sha256(want).hexdigest())

    def test_palette_ppm_exact(self):
        pal = bytes(b for i in range(256) for b in ((i * 7) & 255, 255 - i, (i * 3) & 255))
        palpath = PLAY / "sprite.pal"
        palpath.write_bytes(pal)
        out = PLAY / "sprite-rgb.ppm"
        r = run("sprite", str(self.fixture), "--out", str(out), "--palette", str(palpath),
                "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        want = ppm(PIX0, pal, 4, 3)
        self.assertEqual(out.read_bytes(), want)
        m = SPRITE_RE.match(r.stdout.strip())
        self.assertEqual(m.group(9), hashlib.sha256(want).hexdigest())

    def test_bank_palette_ppm_exact(self):
        fixture = PLAY / "sprite-pal.pvi"
        build_palette_sprite(fixture)
        out = PLAY / "sprite-bankpal.ppm"
        r = run("sprite", str(fixture), "--name", "pal.fsh", "--out", str(out),
                "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("source=bank", r.stdout)
        want = ppm(b"\x00", PAL8, 1, 1)
        self.assertEqual(out.read_bytes(), want)

    def test_dump_palette_writes_rgb8(self):
        fixture = PLAY / "sprite-pal.pvi"
        build_palette_sprite(fixture)
        dump = PLAY / "sprite-bankpal.rgb"
        out = PLAY / "sprite-bankpal2.ppm"
        r = run("sprite", str(fixture), "--name", "pal.fsh", "--out", str(out),
                "--dump-palette", str(dump), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(dump.read_bytes(), PAL8)

    def test_file_palette_overrides_bank(self):
        fixture = PLAY / "sprite-pal.pvi"
        build_palette_sprite(fixture)
        pal = bytes(b for i in range(256) for b in ((i * 7) & 255, 255 - i, (i * 3) & 255))
        palpath = PLAY / "sprite-override.pal"
        palpath.write_bytes(pal)
        out = PLAY / "sprite-override.ppm"
        r = run("sprite", str(fixture), "--name", "pal.fsh", "--out", str(out),
                "--palette", str(palpath), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("source=file", r.stdout)
        self.assertEqual(out.read_bytes(), ppm(b"\x00", pal, 1, 1))

    def test_player_chunk_is_not_palette(self):
        fixture = PLAY / "sprite-pal.pvi"
        build_palette_sprite(fixture)
        out = PLAY / "sprite-playerchunk.ppm"
        r = run("sprite", str(fixture), "--name", "play.fsh", "--out", str(out),
                "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("source=gray", r.stdout)
        self.assertEqual(out.read_bytes(), ppm(b"\x05", GRAY, 1, 1))

    def test_name_selects_entry_and_frame(self):
        out = PLAY / "sprite-one.ppm"
        r = run("sprite", str(self.fixture), "--name", "one.fsh", "--frame", "0",
                "--out", str(out), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        want = ppm(PIX2, GRAY, 1, 1)
        self.assertEqual(out.read_bytes(), want)
        m = SPRITE_RE.match(r.stdout.strip())
        self.assertEqual(m.groups()[:8],
                         ("2", "1", "one.fsh", "1", "0", "1", "1", "ppm"))

    def test_entry_index_matches_name(self):
        by_index = PLAY / "sprite-index.ppm"
        by_name = PLAY / "sprite-name.ppm"
        r = run("sprite", str(self.fixture), "--entry", "1", "--out", str(by_index))
        self.assertEqual(r.returncode, 0, r.stderr)
        r = run("sprite", str(self.fixture), "--name", "one.fsh", "--out", str(by_name))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(by_index.read_bytes(), by_name.read_bytes())

    def test_frame_out_of_range_fails(self):
        r = run("sprite", str(self.fixture), "--frame", "2", "--out", str(PLAY / "x.ppm"))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("error", r.stderr.lower())

    def test_unknown_name_fails(self):
        r = run("sprite", str(self.fixture), "--name", "nope.fsh", "--out", str(PLAY / "x.ppm"))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("error", r.stderr.lower())

    def test_entry_and_name_exclusive(self):
        r = run("sprite", str(self.fixture), "--entry", "0", "--name", "raw.fsh")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("error", r.stderr.lower())

    def test_short_palette_fails(self):
        palpath = PLAY / "short.pal"
        palpath.write_bytes(b"\0" * 767)
        r = run("sprite", str(self.fixture), "--palette", str(palpath),
                "--out", str(PLAY / "x.ppm"))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("error", r.stderr.lower())

    def test_junk_input_fails(self):
        bad = PLAY / "bad-bigf.pvi"
        bad.write_bytes(b"not a container at all")
        r = run("sprite", str(bad))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("error", r.stderr.lower())

    def test_missing_file_fails(self):
        r = run("sprite", str(PLAY / "nope.pvi"))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("error", r.stderr.lower())


class TestSpriteGolden(unittest.TestCase):
    def test_raw_shpi_entry(self):
        out = PLAY / "net.ppm"
        r = run("sprite", str(GOLDEN / "gameart0.pvi"), "--name", "Net.fsh", "--frame", "0",
                "--out", str(out), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        m = SPRITE_RE.match(r.stdout.strip())
        self.assertIsNotNone(m, r.stdout)
        self.assertEqual((m.group(1), m.group(3), m.group(4), m.group(5)),
                         ("60", "Net.fsh", "5", "0"))
        blob = out.read_bytes()
        w, h, hdr = ppm_info(blob)
        self.assertEqual((str(w), str(h)), (m.group(6), m.group(7)))
        self.assertEqual(len(blob), hdr + w * h * 3)
        self.assertEqual(hashlib.sha256(blob).hexdigest(), m.group(9))

    def test_qfs_entry_reaches_shpi(self):
        out = PLAY / "replay.ppm"
        r = run("sprite", str(GOLDEN / "gameart0.pvi"), "--name", "replay.qfs", "--frame", "0",
                "--out", str(out), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        m = SPRITE_RE.match(r.stdout.strip())
        self.assertIsNotNone(m, r.stdout)
        self.assertEqual((m.group(1), m.group(3), m.group(4), m.group(5)),
                         ("60", "replay.qfs", "13", "0"))
        blob = out.read_bytes()
        w, h, hdr = ppm_info(blob)
        self.assertEqual(len(blob), hdr + w * h * 3)

    def test_entry_index_matches_name(self):
        by_index = PLAY / "net-index.ppm"
        by_name = PLAY / "net-name.ppm"
        r = run("sprite", str(GOLDEN / "gameart0.pvi"), "--entry", "36",
                "--out", str(by_index))
        self.assertEqual(r.returncode, 0, r.stderr)
        r = run("sprite", str(GOLDEN / "gameart0.pvi"), "--name", "Net.fsh",
                "--out", str(by_name))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(by_index.read_bytes(), by_name.read_bytes())


@unittest.skipUnless(ISO.exists(), "game/FIFAPCCD96.iso not present")
class TestSpriteIso(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sys.path.insert(0, str(ROOT / "tools"))
        import fifa96_bind
        PLAY.mkdir(parents=True, exist_ok=True)
        extents, data = fifa96_bind.iso_files(ISO)
        off, size = extents["/ART/PLAYART.PVI"]
        cls.playart = PLAY / "playart.pvi"
        cls.playart.write_bytes(data[off:off + size])

    def test_raw_bank_frame_geometry(self):
        out = PLAY / "xstandd.ppm"
        r = run("sprite", str(self.playart), "--name", "xstandd.fsh", "--frame", "0",
                "--out", str(out), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        m = SPRITE_RE.match(r.stdout.strip())
        self.assertIsNotNone(m, r.stdout)
        self.assertEqual((m.group(1), m.group(3), m.group(4), m.group(5)),
                         ("91", "xstandd.fsh", "5", "0"))
        self.assertEqual((m.group(6), m.group(7)), ("20", "49"))
        blob = out.read_bytes()
        w, h, hdr = ppm_info(blob)
        self.assertEqual((w, h), (20, 49))
        self.assertEqual(len(blob), hdr + w * h * 3)

    def test_full_chain_jump_qfs(self):
        out = PLAY / "jump.ppm"
        r = run("sprite", str(self.playart), "--name", "jump.qfs", "--frame", "0",
                "--out", str(out), "--print-summary")
        self.assertEqual(r.returncode, 0, r.stderr)
        m = SPRITE_RE.match(r.stdout.strip())
        self.assertIsNotNone(m, r.stdout)
        self.assertEqual((m.group(1), m.group(2), m.group(3), m.group(4), m.group(5)),
                         ("91", "4", "jump.qfs", "30", "0"))
        blob = out.read_bytes()
        w, h, hdr = ppm_info(blob)
        self.assertEqual(len(blob), hdr + w * h * 3)
        self.assertEqual(hashlib.sha256(blob).hexdigest(), m.group(9))


class TestAutoContainer(unittest.TestCase):
    def test_video_default_three_frames(self):
        out = PLAY / "auto-video"
        r = run("auto", str(GOLDEN / "vid_game.tgv"), "--class", "video", "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        frames = sorted(out.glob("video/*/frame-*.ppm"))
        self.assertEqual([p.name for p in frames],
                         ["frame-0000.ppm", "frame-0001.ppm", "frame-0002.ppm"])
        self.assertEqual(len([x for x in r.stdout.splitlines() if x.startswith("frame")]), 3)

    def test_video_max_frames(self):
        out = PLAY / "auto-video-max"
        r = run("auto", str(GOLDEN / "vid_game.tgv"), "--class", "video", "--max-frames", "1",
                "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(sorted(p.name for p in out.glob("video/*/frame-*.ppm")),
                         ["frame-0000.ppm"])

    def test_eacs_writes_wav(self):
        raw = (EACS / "bank-h.eacs").read_bytes()
        rate = struct.unpack_from("<I", raw, 4)[0]
        data_off = struct.unpack_from("<I", raw, 0x18)[0] or 0x20
        out = PLAY / "auto-eacs"
        r = run("auto", str(EACS / "bank-h.eacs"), "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual((out / "audio/sample.wav").read_bytes(), wav(raw[data_off:], rate, 2))

    def test_bnk_first_id(self):
        out = PLAY / "auto-bnk"
        r = run("auto", str(GOLDEN / "sfx_game.bnk"), "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("id=1", r.stdout)
        _, _, _, blob = wav_fields(out / "audio/sample.wav")
        self.assertEqual(hashlib.sha256(blob).hexdigest(), AUDIO_SHA["bnk1"])

    def test_bnk_entry_selects_id(self):
        out = PLAY / "auto-bnk-id"
        r = run("auto", str(GOLDEN / "sfx_game.bnk"), "--class", "audio", "--entry", "2",
                "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("id=2", r.stdout)

    def test_crd_summary(self):
        r = run("auto", str(GOLDEN / "audio/crd-crd0.crd"), "--out", str(PLAY / "auto-crd"))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("auto crd", r.stdout)
        self.assertIn("tracks=4", r.stdout)
        self.assertIn("events=16", r.stdout)

    def test_sprite_picks_first_decodable(self):
        out = PLAY / "auto-art"
        r = run("auto", str(GOLDEN / "gameart0.pvi"), "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        blob = (out / "art/sprite.ppm").read_bytes()
        line = [x for x in r.stdout.splitlines() if x.startswith("sprite:")][0]
        self.assertEqual(hashlib.sha256(blob).hexdigest(), line.split("sha256=")[1])
        ppm_info(blob)

    def test_qfs_envelope_unwraps_to_sprite(self):
        out = PLAY / "auto-qfs"
        r = run("auto", str(GOLDEN / "fw1.qfs"), "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("decoded kind=shpi", r.stdout)
        ppm_info((out / "art/sprite.ppm").read_bytes())

    def test_bare_file_is_auto(self):
        out = PLAY / "auto-bare"
        r = run(str(GOLDEN / "eacs/bank-h.eacs"), "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertTrue((out / "audio/sample.wav").exists())

    def test_list_writes_nothing(self):
        out = PLAY / "auto-list"
        r = run("auto", str(GOLDEN / "gameart0.pvi"), "--list", "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("kind=envelope", r.stdout)
        self.assertIn("envelope:", r.stdout)
        self.assertFalse(out.exists())


@unittest.skipUnless(ISO.exists(), "game/FIFAPCCD96.iso not present")
class TestAutoIso(unittest.TestCase):
    def test_list_counts(self):
        r = run("auto", str(ISO), "--list")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("iso9660 bytes=476033024 files=798", r.stdout)
        for name, count in ISO_COUNTS.items():
            self.assertIn("class %s files=%d" % (name, count), r.stdout)

    def test_smoke_pass(self):
        out = PLAY / "auto-iso"
        r = run("auto", str(ISO), "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("auto video /VIDEO/VID_ALIE.TGV", r.stdout)
        self.assertIn("auto audio bnk /SOUND/CHN_ARG0.BNK id=41", r.stdout)
        self.assertIn("auto crd /SOUND/CRD_CRD0.CRD", r.stdout)
        self.assertIn("auto audio viv /SOUND/PHR_1HF0.VIV", r.stdout)
        self.assertIn("sprite: entries=60 entry=35 name=Npost.fsh", r.stdout)
        frame = (out / "video/VID_ALIE/frame-0000.ppm").read_bytes()
        self.assertEqual(hashlib.sha256(frame).hexdigest(), ISO_VIDEO0_SHA)
        audio = (out / "audio/CHN_ARG0.wav").read_bytes()
        self.assertEqual(hashlib.sha256(audio).hexdigest(), ISO_AUDIO0_SHA)
        sprite = (out / "art/GAMEART0.ppm").read_bytes()
        self.assertEqual(hashlib.sha256(sprite).hexdigest(), ISO_SPRITE0_SHA)
        ppm_info(frame)
        ppm_info(sprite)
        wav_fields(out / "audio/CHN_ARG0.wav")

    def test_class_video_modex(self):
        out = PLAY / "auto-iso-modex"
        r = run("auto", str(ISO), "--class", "video", "--max-frames", "1", "--modex",
                "--out", str(out))
        self.assertEqual(r.returncode, 0, r.stderr)
        blob = (out / "video/VID_ALIE/frame-0000.modex").read_bytes()
        self.assertEqual(len(blob), 100 * 80 * 4)


if __name__ == "__main__":
    unittest.main(verbosity=2)
