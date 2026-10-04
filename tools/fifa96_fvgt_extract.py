#!/usr/bin/env python3
"""fifa96_fvgt_extract.py — extract the FU-31 fVGT golden frame.

Parses a guest RAM dump taken during the intro video run of the patched
ISO and recovers the single fVGT frame captured by the return cave at
``vgt_decode_f``'s epilogue (0xAE20B).

The FU-31 cave has no entry probe, so there is no live T_PROBE frame to
derive the guest load delta from. Extraction therefore uses the load delta
observed in every FU-4/FU-20/FU-21 session (``0x1FC000``, overridable with
``--delta-load``) and fails loudly if the block header at the mapped
address is not the expected ``'FVGT'`` block.

Golden block layout (same as FU-20/21):
    +0x00 next=0, +0x04 method=0x66, +0x08 out_len, +0x0C 'FVGT' magic,
    +0x10 in_cap input chunk slice, +0x10+in_cap out_len canvas bytes.

The chunk (input) header is little-endian (FU-29 errata), tags at
+0x00, total length u32 at +0x04. The four u16 fields are the decoder's
counts, not pixel dimensions (Ghidra ``vgt_decode_f`` 0xADF34..): +0x08
index_count (2x signed-10-bit pairs), +0x0A raw_count (16-byte blocks),
+0x0C palette_count (8-byte records), +0x0E row_bits (row-stream index
width). The canvas copied is the composited surface ``ctx[10]+0x10``
(index 10 = byte 0x28, after vgt_decode_f's entry swap; pitch = ctx[0],
height = ctx[1]); ``out_len`` is pitch*height bytes. The
success bar's ``dims_ok`` verifies the chunk's declared total length
equals the exact size those fields imply for this canvas
(``chunk_size``), which ties the chunk to the canvas.

Writes ``fvgt-01.in.bin`` (the 0x4000 input slice), ``fvgt-01.out.bin``
(the exact decoded canvas) and ``fvgt-01.json`` (chunk facts, checks,
provenance, SHA-256s).
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fifa96_runtime as rt  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
SCRATCH_LINK = 0x6728D
MARKER_LINK = 0x18A01                    # FU-32 pre-pointer handoff cell
IN_CAP = 0x4000                          # max measured fVGT chunk 0x3AB8
OUT_CAP = 0x40000
MAGIC = 0x54475646                       # 'FVGT' golden block magic
METHOD = 0x66                            # fixed fVGT method id
PRE_METHOD = 0x70                        # logical pre record id ('p')
TAG = b"fVGT"                            # chunk tag at chunk+0
DEFAULT_DELTA_LOAD = 0x1FC000            # observed in every FU session


def _u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def _u16(data, off):
    return struct.unpack_from("<H", data, off)[0]


def _bit_bytes(bits):
    """Dword-padded packed-bit extent, matching vgt_decode_f's arithmetic."""
    return ((bits + 0x1F) & ~0x1F) >> 3


def chunk_size(index_count, raw_count, palette_count, row_bits, out_len):
    """Exact in-memory fVGT chunk length for a canvas of `out_len` bytes.

    vgt_decode_f layout: 0x14-byte header/tag, `index_count` pairs of signed
    10-bit fields (dword-padded), `raw_count` 16-byte raw blocks,
    `palette_count` 8-byte palette records, then the row stream: one
    `row_bits`-bit index per 16-byte canvas block (dword-padded).
    """
    if out_len % 16:
        return None
    return (0x14
            + _bit_bytes(index_count * 20)
            + raw_count * 16
            + palette_count * 8
            + _bit_bytes(row_bits * (out_len // 16)))


def block_view(data, ptr, delta_dump, delta_load, in_cap=IN_CAP,
               out_cap=OUT_CAP, magic=MAGIC, method=METHOD):
    """Parse the FU-31 golden block that `ptr` (runtime) maps to.

    Raises ValueError when the pointer does not map inside the dump, the
    header is not the expected single fVGT block, or it is truncated.
    """
    block_off = ptr + delta_dump - delta_load
    if block_off < 0 or block_off + 0x10 > len(data):
        raise ValueError(
            f"golden block 0x{ptr:x} maps outside the dump "
            f"(offset 0x{block_off:x})")
    next_ptr = _u32(data, block_off)
    got_method = _u32(data, block_off + 4)
    out_len = _u32(data, block_off + 8)
    got_magic = _u32(data, block_off + 0xC)
    if got_magic != magic:
        raise ValueError(
            f"golden magic 0x{got_magic:08x} != 0x{magic:08x} at dump "
            f"offset 0x{block_off:x}; block was clobbered, the delta_load "
            f"is wrong, or the pointer is stale")
    if got_method != method:
        raise ValueError(
            f"golden method 0x{got_method:02x} != 0x{method:02x} at dump "
            f"offset 0x{block_off:x}")
    if next_ptr != 0:
        raise ValueError(
            f"golden block next pointer 0x{next_ptr:x} != 0; the FU-31 cave "
            f"stores a single frame")
    if out_len > out_cap:
        raise ValueError(f"golden out_len 0x{out_len:x} over cap 0x{out_cap:x}")
    if block_off + 0x10 + in_cap + out_len > len(data):
        raise ValueError(
            f"golden block 0x{ptr:x} (in_cap 0x{in_cap:x} + out_len "
            f"0x{out_len:x}) is truncated by the dump")
    inp = data[block_off + 0x10:block_off + 0x10 + in_cap]
    out = data[block_off + 0x10 + in_cap:block_off + 0x10 + in_cap + out_len]
    return {"ptr": ptr, "block_off": block_off, "next": next_ptr,
            "method": got_method, "out_len": out_len, "input": inp,
            "output": out}


def extract_frame(data, delta_load=DEFAULT_DELTA_LOAD,
                  scratch_link=SCRATCH_LINK, in_cap=IN_CAP, out_cap=OUT_CAP,
                  magic=MAGIC, method=METHOD):
    """Parse a guest RAM dump into the captured fVGT frame record.

    Raises ValueError when the dump cannot be trusted (no WATCOM banner,
    scratch cell outside the dump or zero, bad block header, chunk
    in_len over the input cap, or any success bar failing).
    """
    delta_dump, anchors = rt.find_delta(data)
    if delta_dump is None:
        raise ValueError("WATCOM banner not found in the dump; cannot map "
                         "link addresses")
    scratch_off = scratch_link + delta_dump
    if scratch_off + 4 > len(data):
        raise ValueError("scratch cell outside the dump")
    ptr = _u32(data, scratch_off)
    if ptr == 0:
        raise ValueError("no fVGT frame captured (scratch pointer is zero)")
    block = block_view(data, ptr, delta_dump, delta_load, in_cap=in_cap,
                       out_cap=out_cap, magic=magic, method=method)
    inp = block["input"]
    tag = inp[0:4]
    in_len = _u32(inp, 4)
    width = _u16(inp, 8)
    height = _u16(inp, 10)
    blockcount = _u16(inp, 12)
    rowcount = _u16(inp, 14)
    if in_len < 16:
        raise ValueError(f"chunk in_len 0x{in_len:x} is below the 16-byte "
                         f"header")
    if in_len > in_cap:
        raise ValueError(f"chunk in_len 0x{in_len:x} over cap 0x{in_cap:x}")
    expected = chunk_size(width, height, blockcount, rowcount,
                          block["out_len"])
    checks = {
        "tag_ok": tag == TAG,
        "dims_ok": (width > 0 and block["out_len"] > 0 and
                    block["out_len"] <= out_cap and
                    expected is not None and in_len == expected),
        "out_nontrivial": (block["out_len"] > 0 and
                           len(set(block["output"])) > 1),
    }
    if not all(checks.values()):
        failed = [k for k, v in checks.items() if not v]
        raise ValueError(
            "success bar failed: " + ", ".join(failed) +
            f" (tag {tag!r}, {width}x{height}, out_len 0x{block['out_len']:x})")
    record = dict(block)
    record.update({
        "tag": tag, "in_len": in_len, "in_cap": in_cap,
        "width": width, "height": height,
        "blockcount": blockcount, "rowcount": rowcount,
        "delta_dump": delta_dump, "delta_load": delta_load,
        "anchors": anchors, "checks": checks,
    })
    return record


def pair_block_view(data, ptr, delta_dump, delta_load, in_cap=IN_CAP,
                    out_cap=OUT_CAP, magic=MAGIC, method=METHOD):
    """Parse the FU-32 paired block that `ptr` (runtime) maps to.

    Layout: ``+0 next`` (unused, capture-latest single block), ``+4 method``
    byte 0x66, ``+8 out_len``, ``+0xC 'FVGT'``, then the in_cap chunk slice,
    the post surface and the pre surface, each out_len bytes. Raises
    ValueError when the pointer does not map inside the dump, the header is
    not the expected pair block, or the block is truncated.
    """
    block_off = ptr + delta_dump - delta_load
    if block_off < 0 or block_off + 0x10 > len(data):
        raise ValueError(
            f"golden block 0x{ptr:x} maps outside the dump "
            f"(offset 0x{block_off:x})")
    out_len = _u32(data, block_off + 8)
    got_magic = _u32(data, block_off + 0xC)
    got_method = data[block_off + 4]
    if got_magic != magic:
        raise ValueError(
            f"golden magic 0x{got_magic:08x} != 0x{magic:08x} at dump "
            f"offset 0x{block_off:x}; block was clobbered, the delta_load "
            f"is wrong, or the pointer is stale")
    if got_method != method:
        raise ValueError(
            f"golden method 0x{got_method:02x} != 0x{method:02x} at dump "
            f"offset 0x{block_off:x}; the block is not a FU-32 pair block")
    if out_len > out_cap:
        raise ValueError(f"golden out_len 0x{out_len:x} over cap 0x{out_cap:x}")
    end = block_off + 0x10 + in_cap + 2 * out_len
    if end > len(data):
        raise ValueError(
            f"golden block 0x{ptr:x} (in_cap 0x{in_cap:x} + 2 x out_len "
            f"0x{out_len:x}) is truncated by the dump")
    inp = data[block_off + 0x10:block_off + 0x10 + in_cap]
    post = data[block_off + 0x10 + in_cap:
                block_off + 0x10 + in_cap + out_len]
    pre = data[block_off + 0x10 + in_cap + out_len:
               block_off + 0x10 + in_cap + 2 * out_len]
    return {"ptr": ptr, "block_off": block_off, "next": _u32(data, block_off),
            "method": got_method, "out_len": out_len, "input": inp,
            "output": post, "pre": pre}


def extract_pair(data, delta_load=DEFAULT_DELTA_LOAD,
                 scratch_link=SCRATCH_LINK, marker_link=MARKER_LINK,
                 in_cap=IN_CAP, out_cap=OUT_CAP, magic=MAGIC,
                 method=METHOD):
    """Parse a guest RAM dump into the captured fVGT pre/post pair.

    The block pointer comes from the scratch cell, the pre surface from the
    block itself (copied by the return cave from the entry-recorded
    pointer); `marker_link` is read for provenance only. Raises ValueError
    when the dump cannot be trusted or any success bar fails.
    """
    delta_dump, anchors = rt.find_delta(data)
    if delta_dump is None:
        raise ValueError("WATCOM banner not found in the dump; cannot map "
                         "link addresses")
    scratch_off = scratch_link + delta_dump
    if scratch_off + 4 > len(data):
        raise ValueError("scratch cell outside the dump")
    ptr = _u32(data, scratch_off)
    if ptr == 0:
        raise ValueError("no fVGT pair captured (scratch pointer is zero)")
    block = pair_block_view(data, ptr, delta_dump, delta_load, in_cap=in_cap,
                            out_cap=out_cap, magic=magic, method=method)
    inp = block["input"]
    tag = inp[0:4]
    in_len = _u32(inp, 4)
    width = _u16(inp, 8)
    height = _u16(inp, 10)
    blockcount = _u16(inp, 12)
    rowcount = _u16(inp, 14)
    if in_len < 16:
        raise ValueError(f"chunk in_len 0x{in_len:x} is below the 16-byte "
                         f"header")
    if in_len > in_cap:
        raise ValueError(f"chunk in_len 0x{in_len:x} over cap 0x{in_cap:x}")
    expected = chunk_size(width, height, blockcount, rowcount,
                          block["out_len"])
    post = block["output"]
    pre = block["pre"]
    checks = {
        "tag_ok": tag == TAG,
        "dims_ok": (width > 0 and block["out_len"] > 0 and
                    block["out_len"] <= out_cap and
                    expected is not None and in_len == expected),
        "out_nontrivial": (block["out_len"] > 0 and
                           len(set(post)) > 1),
        "pre_nontrivial": (block["out_len"] > 0 and len(set(pre)) > 1),
        "pair_differs": pre != post,
    }
    if not all(checks.values()):
        failed = [k for k, v in checks.items() if not v]
        raise ValueError(
            "success bar failed: " + ", ".join(failed) +
            f" (tag {tag!r}, {width}x{height}, out_len 0x{block['out_len']:x})")
    marker_off = marker_link + delta_dump
    marker = _u32(data, marker_off) if marker_off + 4 <= len(data) else None
    record = dict(block)
    record.update({
        "tag": tag, "in_len": in_len, "in_cap": in_cap,
        "width": width, "height": height,
        "blockcount": blockcount, "rowcount": rowcount,
        "delta_dump": delta_dump, "delta_load": delta_load,
        "anchors": anchors, "checks": checks, "marker": marker,
        "layout": "pair",
    })
    return record


def write_golden_pair(record, out_dir, stem="fvgt-01", provenance=None):
    """Write the pair fixture set; returns the stem.

    ``<stem>.in.bin`` is the bounded input slice, ``.out.bin`` the post
    surface, ``.pre.bin`` the pre surface; the JSON carries both records'
    provenance and checks plus a ``layout: pair`` marker.
    """
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    base = out_dir / stem
    inp_path = base.with_suffix(".in.bin")
    out_path = base.with_suffix(".out.bin")
    pre_path = base.with_suffix(".pre.bin")
    inp_path.write_bytes(record["input"])
    out_path.write_bytes(record["output"])
    pre_path.write_bytes(record["pre"])
    prov = dict(provenance or {})
    prov.setdefault("pre", {
        "source": "entry ctx[10] marker",
        "marker_link": f"{MARKER_LINK:#x}",
        "captured_at": "vgt_decode_f return 0xAE20B",
    })
    prov.setdefault("post", {
        "source": "ctx[10]+0x10 at vgt_decode_f return",
        "captured_at": "vgt_decode_f return 0xAE20B",
    })
    meta = {
        "layout": "pair",
        "tag": record["tag"].decode("latin1"),
        "width": record["width"],
        "height": record["height"],
        "blockcount": record["blockcount"],
        "rowcount": record["rowcount"],
        "in_len": record["in_len"],
        "in_cap": record["in_cap"],
        "out_len": record["out_len"],
        "method": record["method"],
        "magic": f"{MAGIC:#010x}",
        "ptr": record["ptr"],
        "marker": record.get("marker"),
        "delta_dump": record["delta_dump"],
        "delta_load": record["delta_load"],
        "checks": record["checks"],
        "pre": {
            "method": PRE_METHOD,
            "out_len": record["out_len"],
            "nontrivial": record["checks"]["pre_nontrivial"],
            "equals_out": not record["checks"]["pair_differs"],
            "sha256": hashlib.sha256(record["pre"]).hexdigest(),
        },
        "provenance": prov,
        "sha256": {
            "in": hashlib.sha256(record["input"]).hexdigest(),
            "out": hashlib.sha256(record["output"]).hexdigest(),
            "pre": hashlib.sha256(record["pre"]).hexdigest(),
        },
    }
    base.with_suffix(".json").write_text(
        json.dumps(meta, indent=2, sort_keys=True) + "\n")
    return base


def write_golden(record, out_dir, stem="fvgt-01", provenance=None):
    """Write the .in.bin/.out.bin/.json fixture triple; returns the stem."""
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    base = out_dir / stem
    inp_path = base.with_suffix(".in.bin")
    out_path = base.with_suffix(".out.bin")
    inp_path.write_bytes(record["input"])
    out_path.write_bytes(record["output"])
    meta = {
        "tag": record["tag"].decode("latin1"),
        "width": record["width"],
        "height": record["height"],
        "blockcount": record["blockcount"],
        "rowcount": record["rowcount"],
        "in_len": record["in_len"],
        "in_cap": record["in_cap"],
        "out_len": record["out_len"],
        "method": record["method"],
        "magic": f"{MAGIC:#010x}",
        "ptr": record["ptr"],
        "delta_dump": record["delta_dump"],
        "delta_load": record["delta_load"],
        "checks": record["checks"],
        "provenance": provenance or {},
        "sha256": {
            "in": hashlib.sha256(record["input"]).hexdigest(),
            "out": hashlib.sha256(record["output"]).hexdigest(),
        },
    }
    base.with_suffix(".json").write_text(
        json.dumps(meta, indent=2, sort_keys=True) + "\n")
    return base


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--dump", required=True, help="guest RAM dump (guest.bin)")
    ap.add_argument("--trace", help="session trace (recorded in provenance)")
    ap.add_argument("--session", help="session id (recorded in provenance)")
    ap.add_argument("--out-dir", default=str(ROOT / "tests" / "golden" / "vgt"))
    ap.add_argument("--stem", default="fvgt-01")
    ap.add_argument("--delta-load", type=lambda s: int(s, 0),
                    default=DEFAULT_DELTA_LOAD,
                    help="guest load delta (default 0x1FC000; FU-31 has no "
                         "entry probe to derive it, so it is validated "
                         "against the block header)")
    ap.add_argument("--scratch-link", type=lambda s: int(s, 0),
                    default=SCRATCH_LINK)
    ap.add_argument("--marker-link", type=lambda s: int(s, 0),
                    default=MARKER_LINK,
                    help="FU-32 pre-pointer cell (pair layout only)")
    ap.add_argument("--pair", action="store_true",
                    help="extract the FU-32 pair block (input + post + pre) "
                         "instead of the FU-31 single-frame block")
    args = ap.parse_args(argv)

    dump = Path(args.dump).read_bytes()
    try:
        if args.pair:
            record = extract_pair(dump, delta_load=args.delta_load,
                                  scratch_link=args.scratch_link,
                                  marker_link=args.marker_link)
        else:
            record = extract_frame(dump, delta_load=args.delta_load,
                                   scratch_link=args.scratch_link)
    except (ValueError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    provenance = {"dump": str(args.dump), "trace": str(args.trace or ""),
                  "session": args.session or ""}
    if args.pair:
        stem = write_golden_pair(record, args.out_dir, stem=args.stem,
                                 provenance=provenance)
        print(f"tag={record['tag'].decode('latin1')} "
              f"dims={record['width']}x{record['height']} "
              f"in_len={record['in_len']} out_len={record['out_len']} "
              f"ptr=0x{record['ptr']:x} marker=0x{record['marker'] or 0:x}")
        print(f"checks={record['checks']}")
        print(f"golden: {stem}.in.bin {stem}.out.bin {stem}.pre.bin "
              f"{stem}.json")
    else:
        stem = write_golden(record, args.out_dir, stem=args.stem,
                            provenance=provenance)
        print(f"tag={record['tag'].decode('latin1')} "
              f"dims={record['width']}x{record['height']} "
              f"in_len={record['in_len']} out_len={record['out_len']} "
              f"ptr=0x{record['ptr']:x}")
        print(f"checks={record['checks']}")
        print(f"golden: {stem}.in.bin {stem}.out.bin {stem}.json")
    return 0


if __name__ == "__main__":
    sys.exit(main())
