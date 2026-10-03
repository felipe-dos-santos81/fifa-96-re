#!/usr/bin/env python3
"""fifa96_vgt_capture.py — FU-20 VGT golden-record capture and extraction.

Two subcommands:

``run``
    Launch DOSBox-X with the capture TSR + serial trace and a patched ISO,
    wait for the entry-hook probe frames, then read the DOSBox-X process
    image through ``process_vm_readv`` (/proc/<pid>/mem) and save the guest
    RAM region that contains the LE image (located by the WATCOM banner).

``extract``
    Parse ``guest.bin`` + ``trace.bin``, require live entry-hook T_PROBE
    frames (descriptor liveness before trusting a dump), resolve the golden
    block pointer from the scratch cell (link 0x6728D), verify the success
    bar and write ``tests/golden/vgt/record-<method>.{in,out}.bin``.

The guest-RAM dump is only readable while DOSBox-X runs; the earlier FU-4/FU-7
experiments used a dumpable DOSBox-X build (``/tmp/opencode/dosbox-x-nocap``)
or ``LD_PRELOAD=/tmp/opencode/dumpable.so`` to bypass ``prctl(PR_SET_DUMPABLE)``.
Set ``DOSBOX_X`` to override the binary (default ``dosbox-x``).
"""
import argparse
import ctypes
import json
import os
import re
import shutil
import signal
import struct
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fifa96_probe as probe  # noqa: E402
import fifa96_runtime as rt  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
SECTOR = 2048

SCRATCH_LINK = 0x6728D
ENTRY_TARGET = 0x9E718
ENTRY_OVERWRITE = 7
ENTRY_SITE = 1
IN_CAP = 0x4400
OUT_CAP = 0x4000
MIN_OUT = 0x20
VGT_MAGIC = 0x54475646
KNOWN_METHODS = {0x10, 0x16, 0x32, 0x34, 0x46, 0x60, 0x62, 0x66,
                 0x6A, 0x6E, 0x70, 0x72, 0x7A}
NEEDLE = rt.BANNER


class _IOVec(ctypes.Structure):
    _fields_ = [("iov_base", ctypes.c_void_p), ("iov_len", ctypes.c_size_t)]


def _libc():
    return ctypes.CDLL("libc.so.6", use_errno=True)


def read_mem(pid, addr, size):
    """Read a process address range; None when unreadable."""
    buf = ctypes.create_string_buffer(size)
    local = _IOVec(ctypes.cast(buf, ctypes.c_void_p), size)
    remote = _IOVec(ctypes.c_void_p(addr), size)
    n = _libc().process_vm_readv(pid, ctypes.byref(local), 1,
                                 ctypes.byref(remote), 1, ctypes.c_ulong(0))
    return buf.raw[:n] if n >= 0 else None


def readable_regions(pid, min_size=4 * 1024 * 1024):
    out = []
    with open(f"/proc/{pid}/maps") as fh:
        for line in fh:
            m = re.match(r"([0-9a-f]+)-([0-9a-f]+) ([rwxps-]{4})",
                         line.rstrip())
            if not m or "r" not in m.group(3):
                continue
            start, end = int(m.group(1), 16), int(m.group(2), 16)
            if end - start >= min_size:
                out.append((start, end))
    return out


def find_guest_region(pid, needle=NEEDLE):
    """Return (start, end) of the readable region containing `needle`."""
    for start, end in readable_regions(pid):
        data = read_mem(pid, start, end - start)
        if data and needle in data:
            return start, end
    return None


def success_bar(record):
    """Evaluate the FU-20 success bar against a parsed golden record."""
    inp = record["input"]
    out = record["output"]
    return {
        "signature": len(inp) >= 5 and inp[1] == 0xFB,
        "method_known": (record["method"] & 0xFE) in KNOWN_METHODS,
        "length_matches": record["header24"] == record["out_len"],
        "output_nontrivial": (MIN_OUT <= record["out_len"] <= OUT_CAP
                              and len(set(out)) > 1),
    }


def extract(data, trace, scratch_link=SCRATCH_LINK,
            entry_target=ENTRY_TARGET, entry_overwrite=ENTRY_OVERWRITE,
            site_id=ENTRY_SITE, in_cap=IN_CAP, out_cap=OUT_CAP,
            magic=VGT_MAGIC):
    """Parse a guest RAM dump + trace into a golden record dict.

    Raises ValueError with a specific reason when the dump cannot be trusted
    (no live descriptor frames, zero scratch, bad magic, out-of-bounds
    pointer) or when the success bar fails.
    """
    frames = probe.probe_frames(trace)
    live = probe.site_frames(frames, site_id)
    if not live:
        raise ValueError(
            f"no T_PROBE descriptor frames for site {site_id}: the entry "
            f"hook is not live, refusing to trust the dump")
    delta_load = probe.load_delta(frames, entry_target, entry_overwrite)
    delta_dump, anchors = rt.find_delta(data)
    if delta_dump is None:
        raise ValueError("WATCOM banner not found in the dump; cannot map "
                         "link addresses")
    scratch_off = scratch_link + delta_dump
    if scratch_off + 4 > len(data):
        raise ValueError("scratch cell outside the dump")
    ptr = struct.unpack_from("<I", data, scratch_off)[0]
    if ptr == 0:
        raise ValueError("scratch pointer is zero: no record was captured "
                         "(hook never matched a small record)")
    block_off = ptr + delta_dump - delta_load
    if block_off < 0 or block_off + 0x10 + in_cap + out_cap > len(data):
        raise ValueError(
            f"golden block 0x{ptr:x} maps outside the dump "
            f"(offset 0x{block_off:x})")
    header = data[block_off:block_off + 0x10]
    got_magic = struct.unpack_from("<I", header, 0)[0]
    if got_magic != magic:
        raise ValueError(
            f"golden magic 0x{got_magic:08x} != 0x{magic:08x} at dump "
            f"offset 0x{block_off:x}; block was clobbered or the pointer "
            f"is stale")
    method = header[4]
    out_len = struct.unpack_from("<I", header, 8)[0]
    if out_len > out_cap:
        raise ValueError(f"golden out_len 0x{out_len:x} over cap 0x{out_cap:x}")
    inp = data[block_off + 0x10:block_off + 0x10 + in_cap]
    out = data[block_off + 0x10 + in_cap:
               block_off + 0x10 + in_cap + out_len]
    header24 = (inp[2] << 16) | (inp[3] << 8) | inp[4]
    record = {
        "method": method, "out_len": out_len, "header24": header24,
        "input": inp, "output": out,
        "ptr": ptr, "block_off": block_off,
        "delta_load": delta_load, "delta_dump": delta_dump,
        "anchors": anchors,
    }
    record["checks"] = success_bar(record)
    if not all(record["checks"].values()):
        failed = [k for k, v in record["checks"].items() if not v]
        raise ValueError(f"success bar failed: {', '.join(failed)} "
                         f"(method 0x{method:02x}, out_len 0x{out_len:x}, "
                         f"header24 0x{header24:x})")
    return record


def write_golden(record, out_dir, provenance=None):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    stem = out_dir / f"record-{record['method']:02x}"
    (stem.with_suffix(".in.bin")).write_bytes(record["input"])
    (stem.with_suffix(".out.bin")).write_bytes(record["output"])
    last = max((i for i, b in enumerate(record["input"]) if b), default=-1)
    meta = {
        "method": record["method"],
        "out_len": record["out_len"],
        "header24": record["header24"],
        "input_slice_len": len(record["input"]),
        "input_last_nonzero": last,
        "checks": record["checks"],
        "ptr": record["ptr"],
        "delta_load": record["delta_load"],
        "delta_dump": record["delta_dump"],
        "provenance": provenance or {},
    }
    (stem.with_suffix(".json")).write_text(
        json.dumps(meta, indent=2, sort_keys=True) + "\n")
    return stem


def _conf_text(iso, cap_file, hdd, keys_lines):
    return f"""[dosbox]
machine=svga_s3
memsize=16

[serial]
serial1=file file:{cap_file} multiplier:100

[sdl]
fullscreen=false

[autoexec]
@echo off
MOUNT C "{hdd}"
IMGMOUNT D "{iso}" -t iso
C:\\FIFACAP.COM
D:
{keys_lines}
FIFA96.EXE
"""


def _keys_lines(keys_file):
    if not keys_file:
        return ""
    check = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "fifa96_keys.py"),
         "--check", str(keys_file)], capture_output=True, text=True)
    if check.returncode != 0:
        raise SystemExit(f"keys file rejected: {check.stderr.strip()}")
    out = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "fifa96_keys.py"),
         str(keys_file)], capture_output=True, text=True)
    if out.returncode != 0:
        raise SystemExit(f"keys expansion failed: {out.stderr.strip()}")
    return out.stdout.strip()


def cmd_run(args):
    cap_dir = ROOT / "captures" / f"session-{args.session}"
    cap_dir.mkdir(parents=True, exist_ok=True)
    hdd = ROOT / "game" / "hdd"
    hdd.mkdir(parents=True, exist_ok=True)
    tsr = ROOT / "build" / "FIFACAP.COM"
    if not tsr.exists():
        raise SystemExit("missing build/FIFACAP.COM (run: make tsr)")
    shutil.copy(tsr, hdd / "FIFACAP.COM")
    iso = Path(args.iso).resolve()
    if not iso.exists():
        raise SystemExit(f"missing ISO: {iso}")
    trace = cap_dir / "trace.bin"
    trace.unlink(missing_ok=True)
    conf = cap_dir / "dosbox.conf"
    conf.write_text(_conf_text(iso, trace, hdd, _keys_lines(args.keys)))
    dosbox = os.environ.get("DOSBOX_X", "dosbox-x")
    proc = subprocess.Popen(
        [dosbox, "-conf", str(conf), "-fastlaunch", "-nopromptfolder",
         "-silent"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print(f"dosbox pid {proc.pid} session {args.session}", flush=True)
    try:
        region = None
        deadline = time.time() + args.wait
        frames = 0
        dumped_at = None
        while time.time() < deadline:
            if proc.poll() is not None:
                raise SystemExit("dosbox exited before the dump")
            if region is None:
                region = find_guest_region(proc.pid)
                if region:
                    print(f"guest region {region[0]:#x}-{region[1]:#x}",
                          flush=True)
            frames = 0
            if trace.exists():
                try:
                    frames = len(probe.site_frames(
                        probe.probe_frames(trace.read_bytes()), ENTRY_SITE))
                except ValueError:
                    frames = -1
            if (region and frames >= args.min_frames and dumped_at is None):
                dumped_at = time.time() + args.settle
                print(f"entry frames={frames}; dumping in {args.settle}s",
                      flush=True)
            if dumped_at is not None and time.time() >= dumped_at:
                break
            time.sleep(1.0)
        if region is None:
            region = find_guest_region(proc.pid)
        if region is None:
            raise SystemExit("guest RAM region not found in the dumpable "
                             "dosbox process")
        data = read_mem(proc.pid, region[0], region[1] - region[0])
        if not data:
            raise SystemExit("guest RAM region became unreadable")
        out = cap_dir / "guest.bin"
        out.write_bytes(data)
        print(f"guest.bin {len(data)} bytes from "
              f"{region[0]:#x}-{region[1]:#x}; entry frames={frames}",
              flush=True)
    finally:
        proc.send_signal(signal.SIGTERM)
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(timeout=5)
    print(f"capture: {cap_dir}", flush=True)
    return 0


def cmd_extract(args):
    dump = Path(args.dump).read_bytes()
    trace = Path(args.trace).read_bytes()
    try:
        record = extract(dump, trace, scratch_link=args.scratch_link,
                         entry_target=args.entry_target)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    stem = write_golden(record, args.out_dir, provenance={
        "dump": str(args.dump), "trace": str(args.trace),
        "session": args.session or "",
    })
    print(f"method=0x{record['method']:02x} out_len={record['out_len']} "
          f"header24=0x{record['header24']:x} ptr=0x{record['ptr']:x}")
    print(f"checks={record['checks']}")
    print(f"golden: {stem}.in.bin {stem}.out.bin {stem}.json")
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run", help="run a dumpable DOSBox-X capture session")
    r.add_argument("--iso", required=True, help="patched ISO copy")
    r.add_argument("--session", required=True)
    r.add_argument("--wait", type=int, default=90,
                   help="seconds to wait for liveness + stability")
    r.add_argument("--settle", type=int, default=5,
                   help="seconds between the min-frames trigger and the dump")
    r.add_argument("--min-frames", type=int, default=1,
                   help="entry T_PROBE frames required before dumping")
    r.add_argument("--keys", help="optional FU-13 keys file")
    r.set_defaults(func=cmd_run)
    e = sub.add_parser("extract", help="extract golden vectors from a dump")
    e.add_argument("--dump", required=True)
    e.add_argument("--trace", required=True)
    e.add_argument("--out-dir", default=str(ROOT / "tests" / "golden" / "vgt"))
    e.add_argument("--session")
    e.add_argument("--scratch-link", type=lambda s: int(s, 0),
                   default=SCRATCH_LINK)
    e.add_argument("--entry-target", type=lambda s: int(s, 0),
                   default=ENTRY_TARGET)
    e.set_defaults(func=cmd_extract)
    args = ap.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
