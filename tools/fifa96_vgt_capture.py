#!/usr/bin/env python3
"""fifa96_vgt_capture.py — FU-21 VGT per-method golden capture/extraction.

Two subcommands:

``run``
    Launch DOSBox-X with the capture TSR + serial trace and a patched ISO,
    wait for the entry-hook probe frames, then read the DOSBox-X process
    image through ``process_vm_readv`` (/proc/<pid>/mem) and save the guest
    RAM region that contains the LE image (located by the WATCOM banner).

``extract``
    Parse ``guest.bin`` + ``trace.bin``, require live entry-hook T_PROBE
    frames (descriptor liveness before trusting a dump), walk the golden
    slot chain from the scratch cell (link 0x6728D) — one block per distinct
    record method byte, copy-once — verify every success bar and write
    ``tests/golden/vgt/record-<method>.{in,out}.bin`` (``--method`` selects).
    FU-20 single-block dumps (record-10) remain extractable.

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
# Valid selectors (stream[0] & 0xFE) per the 0x9E718 dispatch tree:
# 0x10 refpack, 0x16 lz_16fb, 0x30/0x32/0x34 huff, 0x46 tree,
# 0x60/0x62/0x66/0x72 delta_prefix, 0x6A/0x6E literal copy, 0x7A rle.
# (FU-19's table missed 0x30 and listed 0x70, which the tree rejects.)
METHOD_SELECTOR = 0xFE
KNOWN_METHODS = {0x10, 0x16, 0x30, 0x32, 0x34, 0x46, 0x60, 0x62, 0x66,
                 0x6A, 0x6E, 0x72, 0x7A}
# Methods whose decoded length is proven to equal the BE24 header length:
# 0x10 (record-10) and 0x6A/0x6E (dispatch returns EBP=header length via
# `MOV ESI,EBP` before the literal copy at 0x9E82B). For every other method
# header24 is informational only.
LENGTH_CONTRACT_METHODS = {0x10, 0x6A, 0x6E}
MAX_SLOTS = 64
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
    """Evaluate the FU-20 success bar against a parsed golden record.

    `length_matches` is enforced only for methods in
    `LENGTH_CONTRACT_METHODS`; for the others the BE24 header field is not
    the decoded length and the check is informational (omitted).
    """
    inp = record["input"]
    out = record["output"]
    selector = record["method"] & METHOD_SELECTOR
    checks = {
        "signature": len(inp) >= 5 and inp[1] == 0xFB,
        "method_known": selector in KNOWN_METHODS,
        "output_nontrivial": (MIN_OUT <= record["out_len"] <= OUT_CAP
                              and len(set(out)) > 1),
    }
    if selector in LENGTH_CONTRACT_METHODS:
        checks["length_matches"] = record["header24"] == record["out_len"]
    return checks


def observed_methods(trace, site_id=ENTRY_SITE):
    """Sorted method bytes observed in live descriptor frames.

    FU-21 entry frames carry the raw record header byte (stream[0]) in the
    caller_lo field, so the trace is a per-method observation record even for
    records the size filter excluded. Only valid for FU-21-patched runs; a
    FU-20 trace has caller-address halves there.
    """
    frames = probe.probe_frames(trace)
    return sorted({f["caller_lo"] & 0xFF for f in probe.site_frames(frames,
                                                                   site_id)})


def _block_view(data, ptr, delta_dump, delta_load, in_cap, out_cap, magic):
    """Parse one self-describing golden block; returns its record view.

    Understands both FU-20 (magic at +0, no linked list) and FU-21 (next at
    +0, magic at +0xC) headers. Raises ValueError when the block does not map
    inside the dump or fails the magic/length checks.
    """
    block_off = ptr + delta_dump - delta_load
    if block_off < 0 or block_off + 0x10 + in_cap + out_cap > len(data):
        raise ValueError(
            f"golden block 0x{ptr:x} maps outside the dump "
            f"(offset 0x{block_off:x})")
    header = data[block_off:block_off + 0x10]
    next_ptr = 0
    legacy = False
    if struct.unpack_from("<I", header, 0)[0] == magic:
        legacy = True
    else:
        got_magic = struct.unpack_from("<I", header, 0xC)[0]
        if got_magic != magic:
            raise ValueError(
                f"golden magic 0x{got_magic:08x} != 0x{magic:08x} at dump "
                f"offset 0x{block_off:x}; block was clobbered or the pointer "
                f"is stale")
        next_ptr = struct.unpack_from("<I", header, 0)[0]
    method = header[4]
    out_len = struct.unpack_from("<I", header, 8)[0]
    if out_len > out_cap:
        raise ValueError(f"golden out_len 0x{out_len:x} over cap 0x{out_cap:x}")
    inp = data[block_off + 0x10:block_off + 0x10 + in_cap]
    out = data[block_off + 0x10 + in_cap:
               block_off + 0x10 + in_cap + out_len]
    header24 = (inp[2] << 16) | (inp[3] << 8) | inp[4]
    return {
        "method": method, "out_len": out_len, "header24": header24,
        "input": inp, "output": out, "ptr": ptr, "block_off": block_off,
        "next": next_ptr, "legacy": legacy,
    }


def extract_records(data, trace, scratch_link=SCRATCH_LINK,
                    entry_target=ENTRY_TARGET,
                    entry_overwrite=ENTRY_OVERWRITE, site_id=ENTRY_SITE,
                    in_cap=IN_CAP, out_cap=OUT_CAP, magic=VGT_MAGIC,
                    method=None, max_slots=MAX_SLOTS):
    """Parse a guest RAM dump + trace into the list of golden records.

    Walks the FU-21 slot chain (or the FU-20 single block) from the scratch
    cell and returns records in capture order (oldest first). Raises
    ValueError with a specific reason when the dump cannot be trusted (no
    live descriptor frames, zero scratch, bad magic, out-of-bounds pointer,
    chain cycle, duplicate method slot) or when any success bar fails.
    `method` optionally selects a set of method bytes; requesting a method
    that has no slot is an error.
    """
    frames = probe.probe_frames(trace)
    live = probe.site_frames(frames, site_id)
    if not live:
        raise ValueError(
            f"no T_PROBE descriptor frames for site {site_id}: the entry "
            f"hook is not live, refusing to trust the dump")
    delta_load = probe.load_delta(live, entry_target, entry_overwrite)
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
    blocks = []
    seen = set()
    while ptr:
        if ptr in seen:
            raise ValueError(f"golden slot chain cycle at 0x{ptr:x}")
        seen.add(ptr)
        if len(blocks) >= max_slots:
            raise ValueError(f"golden slot chain longer than {max_slots} "
                             f"blocks; refusing to walk further")
        block = _block_view(data, ptr, delta_dump, delta_load, in_cap,
                            out_cap, magic)
        blocks.append(block)
        if block["legacy"]:
            if len(seen) != 1:
                raise ValueError(
                    f"legacy FU-20 block found mid-chain at 0x{ptr:x}")
            break
        ptr = block["next"]
    records = list(reversed(blocks))
    selectors = [r["method"] & METHOD_SELECTOR for r in records]
    dupes = sorted({m for m in selectors if selectors.count(m) > 1})
    if dupes:
        raise ValueError(
            "duplicate method slot(s) "
            + ", ".join(f"0x{m:02x}" for m in dupes)
            + ": copy-once per slot is violated")
    wanted = None if method is None else {m & METHOD_SELECTOR for m in method}
    for record in records:
        record["delta_load"] = delta_load
        record["delta_dump"] = delta_dump
        record["anchors"] = anchors
        record["checks"] = success_bar(record)
        if wanted is not None and \
                (record["method"] & METHOD_SELECTOR) not in wanted:
            continue
        if not all(record["checks"].values()):
            failed = [k for k, v in record["checks"].items() if not v]
            raise ValueError(
                f"success bar failed: {', '.join(failed)} "
                f"(method 0x{record['method']:02x}, "
                f"out_len 0x{record['out_len']:x}, "
                f"header24 0x{record['header24']:x})")
    if wanted is not None:
        missing = sorted(m for m in wanted if m not in selectors)
        if missing:
            raise ValueError(
                "requested method(s) not captured: "
                + ", ".join(f"0x{m:02x}" for m in missing))
        records = [r for r in records
                   if (r["method"] & METHOD_SELECTOR) in wanted]
    return records


def extract(data, trace, method=None, **kwargs):
    """FU-20-compatible single-record wrapper around `extract_records`.

    Without `method` the dump must hold exactly one record; with `method`
    the selected record is returned.
    """
    records = extract_records(data, trace, method=method, **kwargs)
    if method is None and len(records) != 1:
        raise ValueError(
            f"dump holds {len(records)} golden slots; pass method=... to "
            f"select one")
    return records[0]


def write_golden(record, out_dir, provenance=None):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    selector = record["method"] & METHOD_SELECTOR
    stem = out_dir / f"record-{selector:02x}"
    (stem.with_suffix(".in.bin")).write_bytes(record["input"])
    (stem.with_suffix(".out.bin")).write_bytes(record["output"])
    last = max((i for i, b in enumerate(record["input"]) if b), default=-1)
    meta = {
        "method": selector,
        "method_raw": record["method"],
        "length_contract": selector in LENGTH_CONTRACT_METHODS,
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
    methods = set(args.method) if args.method else None
    try:
        records = extract_records(dump, trace, scratch_link=args.scratch_link,
                                  entry_target=args.entry_target,
                                  method=methods)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    for record in records:
        stem = write_golden(record, args.out_dir, provenance={
            "dump": str(args.dump), "trace": str(args.trace),
            "session": args.session or "",
        })
        print(f"method=0x{record['method']:02x} out_len={record['out_len']} "
              f"header24=0x{record['header24']:x} ptr=0x{record['ptr']:x}")
        print(f"checks={record['checks']}")
        print(f"golden: {stem}.in.bin {stem}.out.bin {stem}.json")
    seen = observed_methods(trace)
    print("observed_methods=[" + ", ".join(f"0x{m:02x}" for m in seen) + "]")
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
    e.add_argument("--method", action="append", type=lambda s: int(s, 0),
                   help="only extract this method byte (repeatable; any "
                        "requested method with no slot is an error)")
    e.set_defaults(func=cmd_extract)
    args = ap.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
