#!/usr/bin/env bash
# build_android.sh — Android arm64-v8a 交叉编译 & APK 打包脚本
#
# 用途：
#   使用 Qt for Android 工具链交叉编译本项目，并打包为 APK。
#
# 用法：
#   ./build_android.sh                                    # 默认 Release
#   BUILD_TYPE=Debug ./build_android.sh                   # Debug APK
#   QT_ANDROID_DIR=~/Qt/6.6.3/android_arm64_v8a ./build_android.sh
#
# 输出：
#   build/android-arm64/android-build/build/outputs/apk/*.apk
#
# 依赖：
#   - Qt 6.6.3 for Android (android_arm64_v8a)
#   - Android SDK (含 NDK 25.1.8937393)
#   - JDK 17
#   - Gradle (由 androiddeployqt 自动调用)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}"
BUILD_DIR="${PROJECT_DIR}/build/android-arm64"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc)}"

# ── 工具链路径配置（均可通过环境变量覆盖）──────────────────────
# Qt6 Android 安装根目录
QT_ANDROID_DIR="${QT_ANDROID_DIR:-$HOME/Qt/6.6.3/android_arm64_v8a}"
# Android SDK 路径
ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$HOME/Android/Sdk}"
# JDK 路径
JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}"
# Android platform 版本（用于 androiddeployqt）
ANDROID_PLATFORM="${ANDROID_PLATFORM:-android-33}"

# Android NDK 路径（需与 Qt for Android 编译时使用的版本一致）
# MapLibre SDK 使用 NDK 27 编译，必须使用匹配的 NDK 版本
ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-$HOME/Android/Sdk/ndk/27.1.12297006}"

# Qt Android 工具链文件（由 Qt 安装提供，包含交叉编译所需的所有 CMake 配置）
TOOLCHAIN="${QT_ANDROID_DIR}/lib/cmake/Qt6/qt.toolchain.cmake"
export ANDROID_NDK_ROOT

# ── 前置检查 ──────────────────────────────────────────────────
# 确保关键工具链文件存在，避免构建中途失败
if [ ! -f "${TOOLCHAIN}" ]; then
  echo "Error: Qt6 Android toolchain not found: ${TOOLCHAIN}"
  echo "Set QT_ANDROID_DIR to your Qt6 Android installation path."
  exit 1
fi

if [ ! -d "${ANDROID_NDK_ROOT}" ]; then
  echo "Error: Android NDK not found: ${ANDROID_NDK_ROOT}"
  echo "Set ANDROID_NDK_ROOT to your Android NDK path."
  exit 1
fi

echo "=== Android arm64-v8a Build ==="
echo "Qt:     ${QT_ANDROID_DIR}"
echo "SDK:    ${ANDROID_SDK_ROOT}"
echo "JDK:    ${JAVA_HOME}"
echo "NDK:    ${ANDROID_NDK_ROOT}"
echo "Type:   ${BUILD_TYPE}"
echo "Jobs:   ${JOBS}"
echo ""

# 清理旧构建（Android 构建建议全量重建，避免残留文件导致问题）
rm -rf "${BUILD_DIR}"

# MapLibre cmake 配置文件路径（必须显式指定，交叉编译环境下无法自动发现）
MAPLIBRE_DIR="${PROJECT_DIR}/maplibre-native-qt_v3.0.0_Qt6.6.3_Android/arm64-v8a/lib/cmake/QMapLibre"

# ── CMake 配置（交叉编译）─────────────────────────────────────
# 关键参数说明：
#   CMAKE_TOOLCHAIN_FILE — Qt Android 工具链，设置 NDK 编译器、sysroot 等
#   ANDROID_ABI          — 目标架构
#   ANDROID_SDK_ROOT     — SDK 路径（androiddeployqt 需要）
#   ANDROID_NDK_ROOT     — NDK 路径（原生代码编译需要）
#   QMapLibre_DIR        — 强制指定 MapLibre cmake 配置位置
#   QT_ANDROID_BUILD_ALL_ABIs=OFF — 只构建 arm64-v8a，不构建其他架构
cmake -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}" \
  -DANDROID_NDK_ROOT="${ANDROID_NDK_ROOT}" \
  -DQMapLibre_DIR="${MAPLIBRE_DIR}" \
  -DQT_ANDROID_BUILD_ALL_ABIs=OFF

# ── 编译 ──────────────────────────────────────────────────────
cmake --build "${BUILD_DIR}" -j"${JOBS}"

# ── APK 打包 ──────────────────────────────────────────────────
# androiddeployqt 是 Qt 提供的 Android 部署工具，负责：
#   1. 将 .so 文件打包到 libs/ 目录
#   2. 生成 Gradle 构建文件
#   3. 调用 Gradle 构建 APK
DEPLOY_SETTINGS="${BUILD_DIR}/android-build-deployment-settings.json"
DEPLOY_BIN="${QT_ANDROID_DIR}/bin/androiddeployqt"

if [ -f "${DEPLOY_SETTINGS}" ]; then
  if [ ! -x "${DEPLOY_BIN}" ]; then
    echo "Error: androiddeployqt not found at ${DEPLOY_BIN}"
    exit 1
  fi
  echo ""
  echo "=== Packaging APK ==="
  DEPLOY_MODE=""
  if [ "${BUILD_TYPE}" = "Release" ]; then
    DEPLOY_MODE="--release"
  fi

  "${DEPLOY_BIN}" \
    --input "${DEPLOY_SETTINGS}" \
    --output "${BUILD_DIR}/android-build" \
    --android-platform "${ANDROID_PLATFORM}" \
    --jdk "${JAVA_HOME}" \
    ${DEPLOY_MODE} \
    --gradle

  APK_DIR="${BUILD_DIR}/android-build/build/outputs/apk"
  echo ""
  echo "Done: $(find "${APK_DIR}" -name '*.apk' 2>/dev/null || echo "${APK_DIR}")"
else
  echo ""
  echo "Warning: deployment settings not found, skipping APK packaging."
  echo "Build output: ${BUILD_DIR}"
fi
