#!/usr/bin/env python3
"""fifa96_probe.py — decode T_PROBE frames from a capture trace.

Frame wire format (emitted by tsr/fifa96_capture.asm):
    type:u8, seq:u16 LE, len:u16 LE, payload[len]
T_PROBE (0x08) payload = four LE u32: site, caller_lo, caller_hi, target_ret.

Runtime addresses are relocated; the delta is derived per run from
target_ret - (target_link + overwrite) and used to normalize the caller
return address back to link space.
"""
import argparse
import struct
import sys

T_PROBE = 0x08
PROBE_LEN = 16


def iter_frames(data):
    off = 0
    n = len(data)
    while off < n:
        if off + 5 > n:
            raise ValueError(f"truncated frame header at 0x{off:x}")
        ftype = data[off]
        seq, plen = struct.unpack_from("<HH", data, off + 1)
        off += 5
        if off + plen > n:
            raise ValueError(f"truncated payload at 0x{off:x}")
        yield ftype, seq, data[off:off + plen], off
        off += plen


def probe_frames(data):
    out = []
    for ftype, _seq, payload, off in iter_frames(data):
        if ftype != T_PROBE or len(payload) != PROBE_LEN:
            continue
        site, lo, hi, tgt = struct.unpack("<IIII", payload)
        out.append({"offset": off, "site": site, "caller_lo": lo,
                    "caller_hi": hi, "target_ret": tgt})
    return out


def normalize(frame, target_link, overwrite):
    n = dict(frame)
    n["caller_ret"] = frame["caller_lo"] | (frame["caller_hi"] << 16)
    n["delta"] = frame["target_ret"] - (target_link + overwrite)
    n["caller_link"] = n["caller_ret"] - n["delta"]
    n["target_ret_link"] = target_link + overwrite
    return n


def main(argv=None):
    ap = argparse.ArgumentParser(description="decode T_PROBE frames")
    ap.add_argument("trace")
    ap.add_argument("--target-link", type=lambda s: int(s, 0), default=None)
    ap.add_argument("--overwrite", type=int, default=None)
    ap.add_argument("--expect-site", type=lambda s: int(s, 0), default=None)
    args = ap.parse_args(argv)
    try:
        with open(args.trace, "rb") as fh:
            data = fh.read()
        frames = probe_frames(data)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    for f in frames:
        if args.target_link is not None and args.overwrite is not None:
            n = normalize(f, args.target_link, args.overwrite)
            print(f'T_PROBE site={f["site"]} caller=0x{n["caller_ret"]:08x} '
                  f'delta=0x{n["delta"]:x} caller_link=0x{n["caller_link"]:x} '
                  f'target_link=0x{n["target_ret_link"]:x}')
        else:
            print(f'T_PROBE site={f["site"]} caller_lo=0x{f["caller_lo"]:04x} '
                  f'caller_hi=0x{f["caller_hi"]:04x} '
                  f'target_ret=0x{f["target_ret"]:08x}')
    print(f"probe_frames={len(frames)}")
    if args.expect_site is not None:
        hit = any(f["site"] == args.expect_site for f in frames)
        print(f"expect_site=0x{args.expect_site:x} hit={hit}")
        return 0 if hit else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
