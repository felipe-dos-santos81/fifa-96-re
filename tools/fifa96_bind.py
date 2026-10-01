#!/usr/bin/env python3
"""fifa96_bind.py — bind capture-rig FILE records to their ISO source bytes.

Reads a trace.bin produced by the P0 capture rig, walks the FILE frames, and
matches every AH=3F read to a byte range of the ISO file its handle was opened
with: the record's `head` bytes are searched in the candidate file and the
record's FNV-1a-32 `hash` is recomputed over the read length to confirm.

Usage:
  fifa96_bind.py TRACE --iso game/FIFAPCCD96.iso [--out report.txt]
  fifa96_bind.py TRACE --dir /path/to/extracted/files

The --dir mode is also what the unit tests exercise (no xorriso needed).
xorriso is required for --iso mode.
"""
import argparse
import os
import sys

TYPE_FILE = 0x02

# FILE payload flags
FLAG_HASH_VALID = 0x01
FLAG_DATA_IS_NAME = 0x02
FLAG_DATA_IS_BYTES = 0x04


def fnv1a32(data):
    h = 2166136261
    for b in data:
        h ^= b
        h = (h * 16777619) & 0xFFFFFFFF
    return h


def walk_frames(data):
    """Yield (type, seq, payload); skip one byte on malformed frames."""
    i = 0
    n = len(data)
    while i + 5 <= n:
        t = data[i]
        seq = data[i + 1] | (data[i + 2] << 8)
        ln = data[i + 3] | (data[i + 4] << 8)
        if t < 1 or t > 7 or i + 5 + ln > n:
            i += 1
            continue
        yield t, seq, data[i + 5:i + 5 + ln]
        i += 5 + ln


def parse_file(payload):
    if len(payload) < 18:
        return None
    dlen = payload[16] | (payload[17] << 8)
    if len(payload) < 18 + dlen:
        return None
    return {
        "ah": payload[0],
        "bx": payload[1] | (payload[2] << 8),
        "cx": payload[3] | (payload[4] << 8),
        "ds": payload[5] | (payload[6] << 8),
        "dx": payload[7] | (payload[8] << 8),
        "ax_after": payload[9] | (payload[10] << 8),
        "flags": payload[11],
        "hash": int.from_bytes(payload[12:16], "little"),
        "dlen": dlen,
        "data": bytes(payload[18:18 + dlen]),
        "hash_valid": bool(payload[11] & FLAG_HASH_VALID),
        "is_name": bool(payload[11] & FLAG_DATA_IS_NAME),
        "is_bytes": bool(payload[11] & FLAG_DATA_IS_BYTES),
    }


def parse_trace(data):
    """Return the FILE records in stream order."""
    out = []
    for t, _seq, payload in walk_frames(data):
        if t != TYPE_FILE:
            continue
        rec = parse_file(payload)
        if rec is not None:
            out.append(rec)
    return out


def normalize_name(name):
    """'D:\\SOUND\\X.BN' / '\\FIFA96.EXE' -> '/SOUND/X.BN' / '/FIFA96.EXE'."""
    s = name.replace("\\", "/")
    if len(s) >= 2 and s[1] == ":":
        s = s[2:]
    if not s.startswith("/"):
        s = "/" + s
    return s


def resolve_handles(records):
    """handle -> opened path, from successful AH=3D name records.

    NOTE: handles are reused; this final-state map is only meaningful for
    single-open streams (and tests). sequential_bind() is the real walk.
    """
    names = {}
    for rec in records:
        if rec["ah"] == 0x3D and rec["is_name"] and rec["dlen"] > 0:
            names[rec["ax_after"]] = rec["data"].rstrip(b"\x00").decode(
                "latin-1")
    return names


def open_names(records):
    """Every distinct opened path in the stream, for ISO prefix collection."""
    out = set()
    for rec in records:
        if rec["ah"] == 0x3D and rec["is_name"] and rec["dlen"] > 0:
            out.add(rec["data"].rstrip(b"\x00").decode("latin-1"))
    return out


def bind_read(rec, names, library, handle_cache=None):
    """Match one AH=3F record to (path, offset, length) in library, or None.

    library: {path: bytes}. handle_cache: optional {handle: (path, bytes)}
    memo so repeated reads of one file do not rescan every candidate.
    """
    if rec["ah"] != 0x3F or not rec["hash_valid"] or rec["dlen"] == 0:
        return None
    name = names.get(rec["bx"])
    if name is None:
        return None
    n = min(rec["cx"], rec["ax_after"])
    if n == 0 or n > 65535:
        return None
    head = rec["data"]

    if handle_cache is not None and rec["bx"] in handle_cache:
        path, blob, last_off, next_off = handle_cache[rec["bx"]]
        for hint in (next_off, last_off):
            if hint and _confirm(blob, head, rec["hash"], n, hint):
                handle_cache[rec["bx"]] = (path, blob, hint, hint + n)
                return (path, hint, n)
        off = _locate(blob, head, rec["hash"], n)
        if off is not None:
            handle_cache[rec["bx"]] = (path, blob, off, off + n)
            return (path, off, n)

    prefix = normalize_name(name).upper()
    for path, blob in library.items():
        if not path.upper().startswith(prefix):
            continue
        off = _locate(blob, head, rec["hash"], n)
        if off is not None:
            if handle_cache is not None:
                handle_cache[rec["bx"]] = (path, blob, off, off + n)
            return (path, off, n)
    return None


MAX_HITS = 64


def _confirm(blob, head, want_hash, n, off):
    return (off >= 0 and off + n <= len(blob)
            and blob.startswith(head, off)
            and fnv1a32(blob[off:off + n]) == want_hash)


def _locate(blob, head, want_hash, n):
    """Find head in blob and confirm fnv1a32(blob[off:off+n]) == want_hash.

    Occurrences are capped so repetitive data (zero runs) cannot cause an
    unbounded scan; sequential reads are resolved by the offset hints in
    bind_read before this is reached.
    """
    hits = 0
    start = 0
    while hits < MAX_HITS:
        off = blob.find(head, start)
        if off < 0:
            return None
        hits += 1
        if off + n <= len(blob) and \
                fnv1a32(blob[off:off + n]) == want_hash:
            return off
        start = off + 1
    return None


def load_dir(root):
    library = {}
    for dirpath, _dirs, files in os.walk(root):
        for f in files:
            full = os.path.join(dirpath, f)
            rel = "/" + os.path.relpath(full, root).replace(os.sep, "/")
            with open(full, "rb") as fh:
                library[rel] = fh.read()
    return library


def iso_files(iso):
    """Parse the ISO9660 directory tree; return ({path: (offset, size)}, data).

    Pure-stdlib walk of the primary volume descriptor and directory records.
    """
    with open(iso, "rb") as fh:
        data = fh.read()
    pvd = data[16 * 2048:17 * 2048]
    if pvd[0] != 1 or pvd[1:6] != b"CD001":
        raise ValueError(f"not an ISO9660 image: {iso}")
    root_len = pvd[156]
    root = pvd[156:156 + root_len]
    root_lba = int.from_bytes(root[2:6], "little")
    root_size = int.from_bytes(root[10:14], "little")

    out = {}
    stack = [("", root_lba, root_size)]
    while stack:
        prefix, lba, size = stack.pop()
        pos = lba * 2048
        end = pos + size
        while pos < end:
            ln = data[pos]
            if ln == 0:
                pos = (pos // 2048 + 1) * 2048
                continue
            rec = data[pos:pos + ln]
            extent = int.from_bytes(rec[2:6], "little")
            dsize = int.from_bytes(rec[10:14], "little")
            flags = rec[25]
            namelen = rec[32]
            name = bytes(rec[33:33 + namelen])
            pos += ln
            if name in (b"\x00", b"\x01"):
                continue
            nm = name.decode("latin-1").split(";")[0]
            full = prefix + "/" + nm
            if flags & 0x02:
                stack.append((full, extent, dsize))
            else:
                out[full] = (extent * 2048, dsize)
    return out, data


def load_iso(iso, prefixes):
    """Bytes of every ISO file whose path starts with a trace name prefix."""
    extents, data = iso_files(iso)
    library = {}
    for path, (off, size) in extents.items():
        if any(path.upper().startswith(p) for p in prefixes):
            library[path] = data[off:off + size]
    return library


def sequential_bind(records, library, samples_per_name=3):
    """Walk the stream in order, tracking open/close per handle, and bind
    every AH=3F read against the handle's current file."""
    handle_cache = {}
    open_of = {}          # handle -> name
    open_records = opens = reads = bound = 0
    by_name = {}
    unbound = {}
    samples = {}

    def row(nm):
        return by_name.setdefault(
            nm, {"iso": set(), "opens": 0, "reads": 0, "bound": 0})

    for rec in records:
        if rec["ah"] == 0x3D:
            open_records += 1
            if rec["is_name"] and rec["dlen"] > 0:
                opens += 1
                nm = rec["data"].rstrip(b"\x00").decode("latin-1")
                open_of[rec["ax_after"]] = nm
                row(nm)["opens"] += 1
        elif rec["ah"] == 0x3E:
            open_of.pop(rec["bx"], None)
        elif rec["ah"] == 0x3F:
            reads += 1
            nm = open_of.get(rec["bx"])
            if nm is None:
                unbound["no-name"] = unbound.get("no-name", 0) + 1
                continue
            row(nm)["reads"] += 1
            got = bind_read(rec, {rec["bx"]: nm}, library, handle_cache)
            if got is None:
                unbound["no-match"] = unbound.get("no-match", 0) + 1
            else:
                bound += 1
                row(nm)["bound"] += 1
                row(nm)["iso"].add(got[0])
                lst = samples.setdefault(nm, [])
                if len(lst) < samples_per_name:
                    lst.append(got)
    return {"open_records": open_records, "opens": opens, "reads": reads,
            "bound": bound, "by_name": by_name, "unbound": unbound,
            "samples": samples}


def report(records, names, library, limit=0):
    return sequential_bind(records, library)


def format_report(res):
    lines = []
    lines.append("FU-2 runtime filename binding")
    lines.append(f"open-records={res['open_records']} opens={res['opens']} "
                 f"reads={res['reads']} reads-bound={res['bound']} "
                 f"unbound={sum(res['unbound'].values())} "
                 f"({res['unbound']})")
    lines.append("")
    lines.append(f"{'trace name':<16} {'iso file':<26} opens reads bound")
    for nm in sorted(res["by_name"]):
        v = res["by_name"][nm]
        iso = sorted(v["iso"])[0] if v["iso"] else "(unmatched)"
        if len(v["iso"]) > 1:
            iso += f" (+{len(v['iso'])-1})"
        lines.append(f"{nm:<16} {iso:<26} {v['opens']:>5} "
                     f"{v['reads']:>5} {v['bound']:>5}")
    if res.get("samples"):
        lines.append("")
        lines.append("sample bindings (path@offset+len):")
        for nm in sorted(res["samples"]):
            for path, off, n in res["samples"][nm]:
                lines.append(f"  {nm} -> {path}@0x{off:06x}+{n}")
    return "\n".join(lines) + "\n"


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("trace")
    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument("--iso")
    src.add_argument("--dir")
    ap.add_argument("--out")
    args = ap.parse_args(argv)

    with open(args.trace, "rb") as fh:
        records = parse_trace(fh.read())

    if args.dir:
        library = load_dir(args.dir)
    else:
        prefixes = {normalize_name(nm).upper()
                    for nm in open_names(records)}
        library = load_iso(args.iso, prefixes)

    res = sequential_bind(records, library)
    text = format_report(res)
    sys.stdout.write(text)
    if args.out:
        with open(args.out, "w") as fh:
            fh.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
