# tools/probe/dump_header.py — read-only, prints magic + size
import sys
p = sys.argv[1]
b = open(p, "rb").read(32)
print(f"{p} len_prefix={b[:16].hex()} ascii={b[:16]!r}")
