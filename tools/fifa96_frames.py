"""Extract PNG frames from DOSBox-X session videos.

Usage: fifa96_frames.py SESSION_DIR [--fps F] [--out DIR] [--list]

With --list, print one AVI path per line (no ffmpeg needed). Otherwise
extract frames from every AVI under SESSION_DIR (recursive) with ffmpeg
and report each frame's ImageMagick mean value, then a final
``avis=N frames=M`` line.
"""
import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

MEAN_RE = re.compile(r"[-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?")


def find_avis(session_dir):
    return sorted(Path(session_dir).rglob("*.avi"))


def ffmpeg_cmd(avi, out_pattern, fps):
    return ["ffmpeg", "-v", "error", "-y", "-i", str(avi),
            "-vf", "fps=%g" % fps, str(out_pattern)]


def parse_identify_mean(text):
    match = MEAN_RE.search(text)
    return float(match.group(0)) if match else None


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Extract frames from a capture session's AVIs.")
    parser.add_argument("session_dir", help="capture session directory")
    parser.add_argument("--fps", type=float, default=1.0,
                        help="frames per second to extract (default 1.0)")
    parser.add_argument("--out", help="output directory (default: "
                        "<session>/frames)")
    parser.add_argument("--list", action="store_true",
                        help="list AVIs and exit")
    args = parser.parse_args(argv)

    session = Path(args.session_dir)
    if not session.is_dir():
        print("error: session directory not found: %s" % session,
              file=sys.stderr)
        return 1
    avis = find_avis(session)
    if not avis:
        print("error: no .avi files under %s" % session, file=sys.stderr)
        return 1
    if args.list:
        for avi in avis:
            print(avi)
        return 0

    for tool in ("ffmpeg", "identify"):
        if shutil.which(tool) is None:
            print("error: %s not found in PATH" % tool, file=sys.stderr)
            return 1

    out_dir = Path(args.out) if args.out else session / "frames"
    out_dir.mkdir(parents=True, exist_ok=True)

    frames = 0
    for avi in avis:
        for stale in out_dir.glob("%s-*.png" % avi.stem):
            stale.unlink()
        pattern = out_dir / ("%s-%%04d.png" % avi.stem)
        if subprocess.run(ffmpeg_cmd(avi, pattern, args.fps)).returncode:
            print("error: ffmpeg failed for %s" % avi, file=sys.stderr)
            return 1
        for frame in sorted(out_dir.glob("%s-*.png" % avi.stem)):
            result = subprocess.run(
                ["identify", "-format", "%[mean]", str(frame)],
                capture_output=True, text=True)
            mean = (parse_identify_mean(result.stdout)
                    if result.returncode == 0 else None)
            if mean is None:
                print("error: identify gave no mean for %s" % frame,
                      file=sys.stderr)
                return 1
            print("frame=%s mean=%s" % (frame.name, mean))
            frames += 1
    print("avis=%d frames=%d" % (len(avis), frames))
    return 0


if __name__ == "__main__":
    sys.exit(main())
