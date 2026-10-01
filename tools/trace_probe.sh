#!/bin/sh
# tools/trace_probe.sh TARGET_LINK SITE_ID SESSION
# Patches a copy of the ISO with a trampoline at TARGET_LINK, runs the game
# headless under the capture TSR, and decodes T_PROBE frames.
set -eu
DIR=$(cd "$(dirname "$0")/.." && pwd)
TARGET=$1
SITE=$2
SESSION=$3
ISO="$DIR/build/fifa96-trace-$SITE.iso"
OW=$(python3 "$DIR/tools/fifa96_patch.py" --iso "$DIR/game/FIFAPCCD96.iso" \
    --target "$TARGET" --print-overwrite)
python3 "$DIR/tools/fifa96_patch.py" --iso "$DIR/game/FIFAPCCD96.iso" \
    --out "$ISO" --target "$TARGET" --cave 0x6728D --site-id "$SITE"
ISO="$ISO" SESSION="$SESSION" HEADLESS=1 TIMEOUT=120 \
    "$DIR/run-fifa96-capture.sh"
python3 "$DIR/tools/fifa96_probe.py" \
    "$DIR/captures/session-$SESSION/trace.bin" \
    --target-link "$TARGET" --overwrite "$OW" --expect-site "$SITE"
