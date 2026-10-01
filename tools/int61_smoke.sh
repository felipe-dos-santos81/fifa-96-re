#!/bin/sh
# tools/int61_smoke.sh — TSR + real-mode INT 61h stub -> serial -> decoder.
set -eu
DIR=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d -t fifa96int61.XXXXXX)
trap 'rm -rf "$WORK"' EXIT
[ -f "$DIR/build/FIFACAP.COM" ] || { echo "missing build/FIFACAP.COM (make tsr)" >&2; exit 1; }
nasm -f bin "$DIR/tsr/probe_stub.asm" -o "$WORK/STUB.COM"
cp "$DIR/build/FIFACAP.COM" "$WORK/"
cat > "$WORK/dosbox.conf" <<EOF
[dosbox]
machine=svga_s3
memsize=16

[serial]
serial1=file file:$WORK/trace.bin multiplier:100

[sdl]
fullscreen=false

[autoexec]
@echo off
MOUNT C "$WORK"
C:
FIFACAP.COM
STUB.COM
exit
EOF
timeout 60 dosbox-x -conf "$WORK/dosbox.conf" -fastlaunch -nopromptfolder -silent >/dev/null 2>&1 || true
echo "--- frames ---"
python3 "$DIR/tools/fifa96_probe.py" "$WORK/trace.bin" --expect-site 0xBEEF
