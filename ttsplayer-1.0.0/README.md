# TTSPlayer 1.0.0

Cross-platform Text-to-Speech player library based on Qt6 and sherpa-onnx.

## Package Contents

### Linux x86_64
- `libttsplayer.so` - TTSPlayer shared library
- `ttsplayer_demo` - Demo application
- `lib/` - Dependencies (sherpa-onnx, onnxruntime)

### Android arm64-v8a
- `libttsplayer.so` - TTSPlayer shared library
- `lib/` - Dependencies (sherpa-onnx, onnxruntime)

### Models
- `models/fr_FR-siwis-low/` - French TTS model (Piper VITS)

## Quick Start

### Linux x86_64 Demo
```bash
# Run French demo
./demo_french.sh

# Or run manually
export LD_LIBRARY_PATH=linux_x86_64/lib:
./linux_x86_64/ttsplayer_demo --model models/fr_FR-siwis-low --text "Bonjour"
```

## Dependencies

- Qt6 Core & Multimedia (Linux/Android system libraries)
- sherpa-onnx (included)
- onnxruntime (included)

## Build Information

- Qt Version: 6.6.3
- Build Date: 2026-05-11 13:46:26
- Platforms: Linux x86_64, Android arm64-v8a
