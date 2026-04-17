#!/usr/bin/env bash
# build_linux.sh — Linux x86_64 构建脚本
#
# 用途：
#   编译本项目的 Linux 桌面版本，生成可执行文件和运行脚本。
#
# 用法：
#   ./build_linux.sh                      # 默认 Release 构建
#   BUILD_TYPE=Debug ./build_linux.sh     # Debug 构建
#   QT_DIR=/opt/Qt/6.6.3/gcc_64 ./build_linux.sh  # 指定 Qt 路径
#
# 输出：
#   build/linux-x86_64/appuntitled  — 可执行文件
#   build/linux-x86_64/run.sh       — 运行脚本（自动设置 LD_LIBRARY_PATH）
#
# 依赖：
#   - Qt 6.6.3 (gcc_64)
#   - CMake 3.16+
#   - C++17 编译器 (GCC 9+ / Clang 10+)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}"
BUILD_DIR="${PROJECT_DIR}/build/linux-x86_64"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc)}"

# Qt6 安装根目录，默认 ~/Qt/6.6.3/gcc_64
QT_DIR="${QT_DIR:-$HOME/Qt/6.6.3/gcc_64}"

# MapLibre Native Qt SDK 路径（Linux 版本）
MAPLIBRE_DIR="${PROJECT_DIR}/maplibre-native-qt_v3.0.0_Qt6.6.3_Linux"

echo "=== Linux x86_64 Build ==="
echo "Qt:     ${QT_DIR}"
echo "Type:   ${BUILD_TYPE}"
echo "Jobs:   ${JOBS}"
echo ""

# ── CMake 配置 ────────────────────────────────────────────────
# CMAKE_PREFIX_PATH 同时包含 Qt 和 MapLibre SDK 路径，
# 让 find_package 能找到 QMapLibre 和 Qt6 组件
cmake -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_PREFIX_PATH="${QT_DIR};${MAPLIBRE_DIR}"

# ── 编译 ──────────────────────────────────────────────────────
cmake --build "${BUILD_DIR}" -j"${JOBS}"

# ── 生成运行脚本 ──────────────────────────────────────────────
# 运行时需要 LD_LIBRARY_PATH 指向 Qt 和 MapLibre 的 .so 目录，
# 否则动态链接器无法找到 Qt 插件和 MapLibre 库
cat > "${BUILD_DIR}/run.sh" <<'RUNEOF'
#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="__QT_LIB__/lib:__MAPLIBRE_LIB__/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$SCRIPT_DIR/appuntitled" "$@"
RUNEOF

# 替换模板变量为实际路径
sed -i "s|__QT_LIB__|${QT_DIR}|g" "${BUILD_DIR}/run.sh"
sed -i "s|__MAPLIBRE_LIB__|${MAPLIBRE_DIR}|g" "${BUILD_DIR}/run.sh"
chmod +x "${BUILD_DIR}/run.sh"

echo ""
echo "Done:"
echo "  Binary: ${BUILD_DIR}/appuntitled"
echo "  Run:    ${BUILD_DIR}/run.sh"
