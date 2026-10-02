#!/usr/bin/env python3
"""fifa96_keys.py — turn a key-step file into DOSBox-X AUTOTYPE lines.

File format (one step per line):
    WAIT KEYS...
WAIT is decimal seconds since the previous step; KEYS are AUTOTYPE tokens
(e.g. `enter`, `esc`, `up`, `kp_8`, commas allowed). Waits are emitted
cumulatively for compatibility; see docs/ghidra/FU13_headless_input.md:
DOSBox-X serializes AUTOTYPE typists, so only a single-step (comma-paced)
keys file is supported and `--check` rejects multi-step files.
"""
import argparse
import sys


class KeysError(ValueError):
    pass


def parse_keys(text):
    steps = []
    for n, line in enumerate(text.splitlines(), 1):
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        parts = s.split(None, 1)
        if len(parts) < 2 or not parts[1].strip():
            raise KeysError(f"line {n}: missing keys")
        try:
            wait = float(parts[0])
        except ValueError:
            raise KeysError(f"line {n}: invalid wait {parts[0]!r}")
        if wait < 0:
            raise KeysError(f"line {n}: negative wait")
        steps.append((wait, parts[1].strip()))
    return steps


def autotype_lines(steps, pace=0.1):
    out = []
    total = 0.0
    for wait, key_text in steps:
        total += wait
        out.append(f"AUTOTYPE -w {total:g} -p {pace:g} {key_text}")
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description="keys file -> AUTOTYPE lines")
    ap.add_argument("file")
    ap.add_argument("--pace", default="0.1")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args(argv)
    try:
        pace = float(args.pace)
        if pace < 0:
            raise ValueError
    except ValueError:
        print(f"error: --pace: invalid pace {args.pace!r}", file=sys.stderr)
        return 1
    try:
        with open(args.file) as fh:
            steps = parse_keys(fh.read())
    except (OSError, KeysError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    if args.check:
        if len(steps) > 1:
            print(
                "error: multi-step keys files are not supported (DOSBox-X "
                "serializes AUTOTYPE typists; see "
                "docs/ghidra/FU13_headless_input.md)",
                file=sys.stderr,
            )
            return 1
        return 0
    if len(steps) > 1:
        print(
            f"warning: {len(steps)} steps; only a single step is "
            "supported (see FU13)",
            file=sys.stderr,
        )
    for line in autotype_lines(steps, pace=pace):
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
