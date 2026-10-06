import hashlib
import re
import struct
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "build" / "fifa96_play"
GOLDEN = ROOT / "tests" / "golden"
VGT = GOLDEN / "vgt"
EACS = GOLDEN / "eacs"
PLAY = ROOT / "build" / "play"
FRAME_RE = re.compile(r"^frame (\d+): (\d+)x(\d+) (ppm|modex) sha256=([0-9a-f]{64})$")
AUDIO_RE = re.compile(r"^audio: frames=(\d+) rate=(\d+) channels=(\d+) sha256=([0-9a-f]{64})$")
AUDIO_SHA = {
    "viv0": "55b8e1b0ea10cfd62db323519f94e8a1af548acd52dc5d62862dbb028b14f3e0",
    "bnk1": "5560690cd27562849ac26618043249cde153d522f3ff2b579f6585952c2272f1",
}


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


class TestHelp(unittest.TestCase):
    def test_help_lists_modes(self):
        r = run("--help")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("video", r.stdout)
        self.assertIn("audio", r.stdout)


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

if __name__ == "__main__":
    unittest.main(verbosity=2)
