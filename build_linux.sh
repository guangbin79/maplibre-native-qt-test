#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}"
BUILD_DIR="${PROJECT_DIR}/build/linux-x86_64"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc)}"

# Qt6 安装根目录，默认 ~/Qt/6.6.3/gcc_64
QT_DIR="${QT_DIR:-$HOME/Qt/6.6.3/gcc_64}"

MAPLIBRE_DIR="${PROJECT_DIR}/maplibre-native-qt_v3.0.0_Qt6.6.3_Linux"

echo "=== Linux x86_64 Build ==="
echo "Qt:     ${QT_DIR}"
echo "Type:   ${BUILD_TYPE}"
echo "Jobs:   ${JOBS}"
echo ""

cmake -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_PREFIX_PATH="${QT_DIR};${MAPLIBRE_DIR}"

cmake --build "${BUILD_DIR}" -j"${JOBS}"

# 生成运行脚本，设置 LD_LIBRARY_PATH 让 Qt 插件能找到 Qt6 和 MapLibre 的 .so
cat > "${BUILD_DIR}/run.sh" <<'RUNEOF'
#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="__QT_LIB__/lib:__MAPLIBRE_LIB__/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$SCRIPT_DIR/appuntitled" "$@"
RUNEOF
sed -i "s|__QT_LIB__|${QT_DIR}|g" "${BUILD_DIR}/run.sh"
sed -i "s|__MAPLIBRE_LIB__|${MAPLIBRE_DIR}|g" "${BUILD_DIR}/run.sh"
chmod +x "${BUILD_DIR}/run.sh"

echo ""
echo "Done:"
echo "  Binary: ${BUILD_DIR}/appuntitled"
echo "  Run:    ${BUILD_DIR}/run.sh"
