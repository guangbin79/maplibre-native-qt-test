#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}"
BUILD_DIR="${PROJECT_DIR}/build/android-arm64"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc)}"

# Qt6 Android 安装根目录
QT_ANDROID_DIR="${QT_ANDROID_DIR:-$HOME/Qt/6.6.3/android_arm64_v8a}"
# Android SDK 路径
ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$HOME/Android/Sdk}"
# JDK 路径
JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}"
# Android platform 版本
ANDROID_PLATFORM="${ANDROID_PLATFORM:-android-33}"

# Android NDK 路径
ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-$HOME/Android/Sdk/ndk/25.1.8937393}"

TOOLCHAIN="${QT_ANDROID_DIR}/lib/cmake/Qt6/qt.toolchain.cmake"
export ANDROID_NDK_ROOT

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

rm -rf "${BUILD_DIR}"
cmake -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}" \
  -DANDROID_NDK_ROOT="${ANDROID_NDK_ROOT}" \
  -DQT_ANDROID_BUILD_ALL_ABIS=OFF

cmake --build "${BUILD_DIR}" -j"${JOBS}"

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
