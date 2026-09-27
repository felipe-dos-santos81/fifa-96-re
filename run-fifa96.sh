#!/bin/sh
set -eu

DIR=$(cd "$(dirname "$0")" && pwd)
ISO="$DIR/game/FIFAPCCD96.iso"
HDD="$DIR/game/hdd"
CONF=$(mktemp -t fifa96.XXXXXX.conf)
trap 'rm -f "$CONF"' EXIT INT TERM

[ -f "$ISO" ] || { echo "missing ISO: $ISO" >&2; exit 1; }
command -v dosbox-x >/dev/null || { echo "dosbox-x not installed (brew install dosbox-x)" >&2; exit 1; }

mkdir -p "$HDD"

cat > "$CONF" <<EOF
[dosbox]
machine=svga_s3
memsize=16

[sdl]
fullscreen=false

[autoexec]
@echo off
MOUNT C "$HDD"
IMGMOUNT D "$ISO" -t iso
D:
FIFA96.EXE
EOF

dosbox-x -conf "$CONF" -fastlaunch -nopromptfolder
