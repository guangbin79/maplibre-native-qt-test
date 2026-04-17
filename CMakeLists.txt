# CMakeLists.txt — 项目构建配置
#
# 本项目是一个跨平台 Qt/QML 地图应用，支持 Linux (x86_64) 和 Android (arm64-v8a)。
#
# 主要依赖：
#   - Qt 6.6+ (Quick, QuickControls2, Positioning, Location)
#   - MapLibre Native Qt SDK (QMapLibre::Core, QMapLibre::Location)
#   - libplugin-HXGISServer.so（本地瓦片服务闭源库）
#
# 目录结构约定：
#   maplibre-native-qt_v3.0.0_Qt6.6.3_Linux/     — Linux 版 MapLibre SDK
#   maplibre-native-qt_v3.0.0_Qt6.6.3_Android/    — Android 版 MapLibre SDK (含 arm64-v8a/)
#   lib-plugin-HXGISServer/                        — HXGIS Server 库 (含 linux/ 和 android/ 子目录)

cmake_minimum_required(VERSION 3.16)

project(untitled VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── 平台判断 ──────────────────────────────────────────────────
# 根据 ANDROID 变量（由 Qt Android 工具链文件设置）选择对应的 SDK 路径。
# ANDROID_ABI 默认为 arm64-v8a（可在命令行覆盖）。
if(ANDROID)
    if(NOT DEFINED ANDROID_ABI)
        set(ANDROID_ABI "arm64-v8a")
    endif()

    # Android SDK 按架构分子目录：maplibre-native-qt_.../arm64-v8a/
    set(MAPLIBRE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native-qt_v3.0.0_Qt6.6.3_Android/${ANDROID_ABI}")
    set(HXGISSERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib-plugin-HXGISServer/plugin-HXGISServer-android/${ANDROID_ABI}")
    set(HXGISSERVER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib-plugin-HXGISServer/plugin-HXGISServer-android")
else()
    # Linux SDK 直接在根目录下
    set(MAPLIBRE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native-qt_v3.0.0_Qt6.6.3_Linux")
    set(HXGISSERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib-plugin-HXGISServer/plugin-HXGISServer-linux/x86_64")
    set(HXGISSERVER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib-plugin-HXGISServer/plugin-HXGISServer-linux")
endif()

# ── 查找依赖 ──────────────────────────────────────────────────
# Android 下必须用 CACHE FORCE 强制指定 QMapLibre_DIR，
# 因为交叉编译环境下 CMake 无法自动发现配置文件。
# Linux 下将 SDK 路径加入 CMAKE_PREFIX_PATH 即可。
if(ANDROID)
    set(QMapLibre_DIR "${MAPLIBRE_SDK_DIR}/lib/cmake/QMapLibre" CACHE PATH "" FORCE)
else()
    list(APPEND CMAKE_PREFIX_PATH "${MAPLIBRE_SDK_DIR}/lib64/cmake/QMapLibre")
endif()

# Qt Location 的 MapLibre 后端需要 Core 和 Location 两个组件
find_package(QMapLibre REQUIRED COMPONENTS Core Location)
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Positioning Location)

qt_standard_project_setup(REQUIRES 6.5)

# ── 构建目标 ──────────────────────────────────────────────────
qt_add_executable(appuntitled
    main.cpp
    hxgisserver.h
    hxgisserver.cpp
)

# ── HXGISServer：头文件 + 链接（Linux & Android 统一处理）────
# 将 lib-plugin-HXGISServer 的头文件目录和库目录加入构建
target_include_directories(appuntitled PRIVATE ${HXGISSERVER_INCLUDE_DIR})
target_link_directories(appuntitled PRIVATE ${HXGISSERVER_DIR})
target_link_libraries(appuntitled PRIVATE plugin-HXGISServer)

# ── 平台特定编译定义 ─────────────────────────────────────────
#   IS_ANDROID       — 代码中用于区分平台逻辑（数据路径、插件路径等）
#   SDK_PLUGINS_PATH — Linux 下 Qt 插件目录（platforms, imageformats 等）
#   SDK_QML_PATH     — Linux 下 MapLibre QML 模块目录
if(ANDROID)
    target_compile_definitions(appuntitled PRIVATE IS_ANDROID)
else()
    set(SDK_PLUGINS_PATH "${MAPLIBRE_SDK_DIR}/plugins")
    set(SDK_QML_PATH "${MAPLIBRE_SDK_DIR}/qml")
    target_compile_definitions(appuntitled PRIVATE
        SDK_PLUGINS_PATH="${SDK_PLUGINS_PATH}"
        SDK_QML_PATH="${SDK_QML_PATH}"
    )
    # Linux 运行时库搜索路径 — 让可执行文件能找到 libplugin-HXGISServer.so
    set_target_properties(appuntitled PROPERTIES BUILD_RPATH "${HXGISSERVER_DIR}")
endif()

# ── QML 模块 ─────────────────────────────────────────────────
# 将 Main.qml 注册为 QML 模块 "untitled"，
# 代码中通过 engine.loadFromModule("untitled", "Main") 加载
qt_add_qml_module(appuntitled
    URI untitled
    QML_FILES Main.qml
)

# ── Qt 链接 ──────────────────────────────────────────────────
target_link_libraries(appuntitled PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Positioning
    Qt6::Location
    QMapLibre::Core
    QMapLibre::Location
)

# 设置 MapLibre Location 插件（将 MapLibre 后端注册为 Qt Location 的地图插件）
qmaplibre_location_setup_plugins(appuntitled)

# ── Android 部署配置 ────────────────────────────────────────
# Android 需要额外配置：
#   - QT_ANDROID_PACKAGE_SOURCE_DIR: Android 资源目录（AndroidManifest.xml 等）
#   - QT_ANDROID_EXTRA_LIBS: 需要打包进 APK 的额外 .so 文件
# Linux/macOS/Windows 设置 GUI 应用属性（无控制台窗口）
if(ANDROID)
    set_target_properties(appuntitled PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_EXTRA_LIBS "${HXGISSERVER_DIR}/libplugin-HXGISServer.so"
    )
else()
    set_target_properties(appuntitled PROPERTIES
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
    )
endif()
