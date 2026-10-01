#!/bin/sh
set -eu
DIR=$(cd "$(dirname "$0")" && pwd)
ISO="$DIR/game/FIFAPCCD96.iso"
HDD="$DIR/game/hdd"
COM="$DIR/build/FIFACAP.COM"
SESSION=${SESSION:-$(date -u +%Y%m%dT%H%M%SZ)}
CAP_DIR="$DIR/captures/session-$SESSION"
CAP_FILE="$CAP_DIR/trace.bin"
CONF=$(mktemp -t fifa96cap.XXXXXX.conf)
trap 'rm -f "$CONF"' EXIT INT TERM

[ -f "$ISO" ] || { echo "missing ISO: $ISO" >&2; exit 1; }
[ -f "$COM" ] || { echo "missing TSR: $COM (run: make tsr)" >&2; exit 1; }
command -v dosbox-x >/dev/null || { echo "dosbox-x not installed" >&2; exit 1; }

mkdir -p "$CAP_DIR" "$HDD"
cp "$COM" "$HDD/FIFACAP.COM"

cat > "$CONF" <<EOF
[dosbox]
machine=svga_s3
memsize=16

[serial]
serial1=file file:$CAP_FILE multiplier:100

[sdl]
fullscreen=false

[autoexec]
@echo off
MOUNT C "$HDD"
IMGMOUNT D "$ISO" -t iso
C:\FIFACAP.COM
D:
FIFA96.EXE
EOF

dosbox-x -conf "$CONF" -fastlaunch -nopromptfolder
printf 'capture: %s\n' "$CAP_FILE"
