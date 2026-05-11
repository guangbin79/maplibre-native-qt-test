#!/bin/bash
# French TTS Demo Script
# Usage: ./demo_french.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODEL_PATH="${SCRIPT_DIR}/models/fr_FR-siwis-low"
DEMO_BIN="${SCRIPT_DIR}/linux_x86_64/ttsplayer_demo"

if [ ! -f "$DEMO_BIN" ]; then
    echo "Error: Demo binary not found at $DEMO_BIN"
    exit 1
fi

if [ ! -d "$MODEL_PATH" ]; then
    echo "Error: French model not found at $MODEL_PATH"
    exit 1
fi

# Set library path
export LD_LIBRARY_PATH="${SCRIPT_DIR}/linux_x86_64/lib:${LD_LIBRARY_PATH}"

echo "========================================"
echo "TTSPlayer French Demo"
echo "========================================"
echo ""
echo "Playing French text: 'Bonjour, comment allez-vous?'"
echo ""

"$DEMO_BIN" --model "$MODEL_PATH" --text "Bonjour, comment allez-vous?"
