#!/bin/sh
set -eu
DIR=$(cd "$(dirname "$0")" && pwd)
ISO=${ISO:-"$DIR/game/FIFAPCCD96.iso"}
HEADLESS=${HEADLESS:-0}
TIMEOUT=${TIMEOUT:-0}
CAPTURE_VIDEO=${CAPTURE_VIDEO:-0}
HDD="$DIR/game/hdd"
COM="$DIR/build/FIFACAP.COM"
SESSION=${SESSION:-$(date -u +%Y%m%dT%H%M%SZ)}
KEYS_FILE=${KEYS_FILE:-}
DRY_RUN=${DRY_RUN:-0}
CAP_DIR="$DIR/captures/session-$SESSION"
CAP_FILE="$CAP_DIR/trace.bin"
VIDEO_DIR="$CAP_DIR/video"
CONF=$(mktemp -t fifa96cap.XXXXXX.conf)
trap 'rm -f "$CONF"' EXIT INT TERM

[ -f "$ISO" ] || { echo "missing ISO: $ISO" >&2; exit 1; }
[ -f "$COM" ] || { echo "missing TSR: $COM (run: make tsr)" >&2; exit 1; }
KEYS_LINES=""
if [ -n "$KEYS_FILE" ]; then
  [ -f "$KEYS_FILE" ] || { echo "missing keys file: $KEYS_FILE" >&2; exit 1; }
  python3 "$DIR/tools/fifa96_keys.py" --check "$KEYS_FILE"
  KEYS_LINES=$(python3 "$DIR/tools/fifa96_keys.py" "$KEYS_FILE")
fi
command -v dosbox-x >/dev/null || { echo "dosbox-x not installed" >&2; exit 1; }

mkdir -p "$CAP_DIR" "$HDD"
CAPTURES_LINES=""
GAME_CMD=FIFA96.EXE
if [ "$CAPTURE_VIDEO" = "1" ]; then
  mkdir -p "$VIDEO_DIR"
  CAPTURES_LINES=$(printf '\ncaptures=%s' "$VIDEO_DIR")
  GAME_CMD="DX-CAPTURE /V FIFA96.EXE"
fi
cp "$COM" "$HDD/FIFACAP.COM"

cat > "$CONF" <<EOF
[dosbox]
machine=svga_s3
memsize=16$CAPTURES_LINES

[serial]
serial1=file file:$CAP_FILE multiplier:100

[sdl]
fullscreen=false

[autoexec]
@echo off
MOUNT C "$HDD"
IMGMOUNT D "$ISO" -t iso
C:\FIFACAP.COM
D:$( [ -n "$KEYS_LINES" ] && printf '\n%s' "$KEYS_LINES" )
$GAME_CMD
EOF

if [ "$DRY_RUN" = "1" ]; then
  cat "$CONF"
  exit 0
fi
if [ "$HEADLESS" = "1" ]; then
  EXTRA="-silent"
else
  EXTRA=""
fi
if [ "$TIMEOUT" -gt 0 ]; then
  timeout -s TERM -k 5 "$TIMEOUT" dosbox-x -conf "$CONF" -fastlaunch -nopromptfolder $EXTRA || true
else
  dosbox-x -conf "$CONF" -fastlaunch -nopromptfolder $EXTRA
fi
printf 'capture: %s\n' "$CAP_FILE"
