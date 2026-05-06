# =============================================================================
# CMakeLists.txt — 项目构建配置
# =============================================================================
#
# 项目概述：
#   本项目是一个跨平台 Qt Widget 地图应用，支持 Linux (x86_64) 和 Android (arm64-v8a)。
#   使用 MapLibre Native Qt SDK 渲染地图，并通过 HXGISServer 插件提供本地瓦片服务。
#
# 主要依赖：
#   - Qt 6.6+ (Widgets, OpenGLWidgets, Network)
#   - MapLibre Native Qt SDK (QMapLibre::Widgets)
#   - libplugin-HXGISServer.so（本地瓦片服务闭源库）
#
# 目录结构约定：
#   项目根目录/
#   ├── CMakeLists.txt                              # 本文件：构建系统主配置
#   ├── main.cpp                                    # 应用程序入口
#   ├── mainwindow.{h,cpp}                          # 主窗口实现
#   ├── mapcontainer.{h,cpp}                        # 地图容器组件
#   ├── scalebarwidget.{h,cpp}                      # 比例尺控件
#   ├── controlpanelwidget.{h,cpp}                  # 控制面板控件
#   ├── hxgisserver.{h,cpp}                         # HXGISServer 封装类
#   ├── android/                                    # Android 资源目录
#   │   └── AndroidManifest.xml                     # Android 应用清单
#   ├── maplibre-native-qt_v3.0.0_Qt6.6.3_Linux/    # Linux 版 MapLibre SDK
#   │   ├── lib64/cmake/QMapLibre/                  # CMake 配置文件
#   │   ├── lib64/libQMapLibre.so                   # 运行时库
#   │   └── plugins/                                # Qt 插件（platforms, imageformats）
#   ├── maplibre-native-qt_v3.0.0_Qt6.6.3_Android/  # Android 版 MapLibre SDK
#   │   └── arm64-v8a/                              # 按 ABI 分目录
#   │       ├── lib/cmake/QMapLibre/                # CMake 配置文件
#   │       └── lib/libQMapLibre.so                 # 运行时库
#   └── lib-plugin-HXGISServer/                     # HXGIS Server 库
#       ├── plugin-HXGISServer-linux/               # Linux 版本
#       │   ├── hxgisserver.h                       # 头文件
#       │   └── x86_64/libplugin-HXGISServer.so     # 64位库文件
#       └── plugin-HXGISServer-android/             # Android 版本
#           ├── hxgisserver.h                       # 头文件
#           └── arm64-v8a/libplugin-HXGISServer.so  # arm64 库文件
#
# 构建命令示例：
#   Linux:   cmake -B build/linux-x86_64 && cmake --build build/linux-x86_64
#   Android: qt-cmake -B build/android -DANDROID_ABI=arm64-v8a && cmake --build build/android
#
# =============================================================================

cmake_minimum_required(VERSION 3.16)

project(untitled VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# 平台检测与 SDK 路径配置
# =============================================================================
#
# 【ANDROID 变量来源】
#   ANDROID 变量由 Qt Android 工具链文件自动设置。当使用 qt-cmake 或指定了
#   Qt 的 Android 工具链（如 qtbase/lib/cmake/Qt6/qt.toolchain.cmake）时，
#   该变量被定义为 TRUE，表示当前是交叉编译到 Android 平台。
#
# 【ANDROID_ABI 说明】
#   ANDROID_ABI 指定目标 Android 设备的 CPU 架构，常见取值：
#     - arm64-v8a：64位 ARM（现代主流 Android 设备）
#     - armeabi-v7a：32位 ARM（兼容旧设备）
#     - x86_64：Intel/AMD 64位（模拟器）
#     - x86：32位 Intel（旧模拟器）
#   默认使用 arm64-v8a，可在命令行通过 -DANDROID_ABI=xxx 覆盖。
#
# 【为什么 Android SDK 需要按架构分目录】
#   Android 是交叉编译平台，不同 ABI 的库文件二进制不兼容。
#   MapLibre SDK 和 HXGISServer 库都为每个 ABI 提供了独立的 .so 文件，
#   因此 SDK 目录按 ABI 分子目录（如 arm64-v8a/），避免链接错误的架构版本。
#
if(ANDROID)
    if(NOT DEFINED ANDROID_ABI)
        set(ANDROID_ABI "arm64-v8a")
    endif()

    # Android SDK 按架构分子目录：maplibre-native-qt_.../arm64-v8a/
    set(MAPLIBRE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native-qt_v3.0.0_Qt6.6.3_Android/${ANDROID_ABI}")
    set(HXGISSERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib-plugin-HXGISServer/plugin-HXGISServer-android/${ANDROID_ABI}")
    set(HXGISSERVER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib-plugin-HXGISServer/plugin-HXGISServer-android")
else()
    # Linux SDK 直接在根目录下（x86_64 是唯一的架构支持）
    set(MAPLIBRE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/maplibre-native-qt_v3.0.0_Qt6.6.3_Linux")
    set(HXGISSERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib-plugin-HXGISServer/plugin-HXGISServer-linux/x86_64")
    set(HXGISSERVER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib-plugin-HXGISServer/plugin-HXGISServer-linux")
endif()

# =============================================================================
# 依赖查找配置
# =============================================================================
#
# 【为什么 Android 需要 CACHE FORCE】
#   在交叉编译环境下，CMake 无法自动搜索到 QMapLibre 的配置文件，原因：
#   1. CMAKE_PREFIX_PATH 在交叉编译时通常指向主机系统的 sysroot，而非目标 SDK
#   2. Android 工具链会修改 find_package 的搜索行为，限制在 NDK 路径内
#   3. 使用 CACHE FORCE 直接设置 QMapLibre_DIR 变量，绕过自动搜索机制，
#      强制指向 Android SDK 中 QMapLibreConfig.cmake 的确切位置
#
# 【为什么 Linux 使用 CMAKE_PREFIX_PATH】
#   Linux 是本地编译，CMake 可以通过 CMAKE_PREFIX_PATH 自动搜索并找到
#   QMapLibreConfig.cmake。这种方式更灵活，允许用户通过环境变量或命令行
#   覆盖 SDK 路径，也符合 CMake 的常规用法。
#
# 【QMapLibre 组件说明】
#   - Core：核心库，提供地图渲染基础功能
#   - Widgets：Qt Widget 封装，提供 QMapLibre::Widgets 目标
#   Widgets 组件内部依赖 Core + Qt6::OpenGLWidgets + Qt6::Widgets
#
if(ANDROID)
    set(QMapLibre_DIR "${MAPLIBRE_SDK_DIR}/lib/cmake/QMapLibre" CACHE PATH "" FORCE)
else()
    list(APPEND CMAKE_PREFIX_PATH "${MAPLIBRE_SDK_DIR}/lib64/cmake/QMapLibre")
endif()

# QMapLibre Widgets 组件（内部依赖 Core + Qt6::OpenGLWidgets + Qt6::Widgets）
find_package(QMapLibre REQUIRED COMPONENTS Core Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Network)

qt_standard_project_setup(REQUIRES 6.5)

# =============================================================================
# 构建目标定义
# =============================================================================
#
# 可执行目标 appuntitled 包含以下源文件：
#   - main.cpp：应用程序入口，初始化 QApplication 和 MainWindow
#   - mainwindow.{h,cpp}：主窗口，包含地图容器和控制面板
#   - mapcontainer.{h,cpp}：地图显示容器，封装 QMapLibre::Map 实例
#   - scalebarwidget.{h,cpp}：比例尺控件，显示当前地图比例
#   - controlpanelwidget.{h,cpp}：控制面板，提供图层切换、缩放等 UI
#   - hxgisserver.{h,cpp}：HXGISServer 封装类，管理本地瓦片服务连接
#
qt_add_executable(appuntitled
    main.cpp
    hxgisserver.h
    hxgisserver.cpp
    mainwindow.h
    mainwindow.cpp
    mapcontainer.h
    mapcontainer.cpp
    scalebarwidget.h
    scalebarwidget.cpp
    controlpanelwidget.h
    controlpanelwidget.cpp
    geojsonbuilder.h
    geojsonbuilder.cpp
    annotationmanager.h
    annotationmanager.cpp
    routegeojsonbuilder.h
    routegeojsonbuilder.cpp
    routemanager.h
    routemanager.cpp
    locationindicatormanager.h
    locationindicatormanager.cpp
    testrunner.h
    testrunner.cpp
)

# =============================================================================
# HXGISServer 库配置
# =============================================================================
#
# 【HXGISServer 是什么】
#   HXGISServer 是一个闭源的本地瓦片服务库，提供离线地图瓦片数据。
#   它作为 Qt 插件形式加载，通过自定义协议（如 hxgis://）向 MapLibre 提供瓦片。
#
# 【配置说明】
#   - target_include_directories：添加头文件搜索路径，使代码中可以 #include "hxgisserver.h"
#   - target_link_directories：添加库文件搜索路径，使链接器能找到 .so 文件
#   - target_link_libraries：链接插件库（实际链接的是 libplugin-HXGISServer.so）
#
# 【平台差异】
#   Linux 和 Android 使用相同的链接方式，但 HXGISSERVER_DIR 和 HXGISSERVER_INCLUDE_DIR
#   根据平台指向不同的子目录（见上文平台检测部分）。
#
target_include_directories(appuntitled PRIVATE ${HXGISSERVER_INCLUDE_DIR})
target_link_directories(appuntitled PRIVATE ${HXGISSERVER_DIR})
target_link_libraries(appuntitled PRIVATE plugin-HXGISServer)

# =============================================================================
# 平台特定编译定义
# =============================================================================
#
# 【IS_ANDROID 宏】
#   用途：在 C++ 代码中通过 #ifdef IS_ANDROID 区分平台逻辑
#   典型使用场景：
#     - 数据文件路径：Android 使用 assets:/，Linux 使用文件系统路径
#     - 插件加载方式：Android 从 APK 内加载，Linux 从文件系统加载
#     - 调试输出：Android 使用 __android_log_print，Linux 使用 std::cout
#
# 【SDK_PLUGINS_PATH 宏】
#   用途：指定 Qt 插件目录路径（platforms, imageformats 等）
#   为什么只在 Linux 定义：
#     - Linux 下 Qt 插件不在标准系统路径，需要显式告诉应用程序去哪里加载
#     - Android 下插件打包在 APK 中，Qt 会自动从 assets 加载，无需额外路径
#   使用方法：在 main.cpp 中通过 QCoreApplication::addLibraryPath() 添加
#
if(ANDROID)
    target_compile_definitions(appuntitled PRIVATE IS_ANDROID)
else()
    set(SDK_PLUGINS_PATH "${MAPLIBRE_SDK_DIR}/plugins")
    target_compile_definitions(appuntitled PRIVATE
        SDK_PLUGINS_PATH="${SDK_PLUGINS_PATH}"
    )
    # Linux 运行时库搜索路径（RPATH）
    #   BUILD_RPATH 指定构建时和运行时的动态库搜索路径
    #   这里设置为 HXGISServer 库所在目录，确保可执行文件能找到 libplugin-HXGISServer.so
    #   如果不设置，运行时会报 "error while loading shared libraries" 错误
    set_target_properties(appuntitled PROPERTIES BUILD_RPATH "${HXGISSERVER_DIR}")
endif()

# =============================================================================
# Qt 与第三方库链接
# =============================================================================
#
# 【链接目标说明】
#   - QMapLibre::Widgets：MapLibre Native Qt SDK 的 Widget 封装
#     * 提供 QMapLibre::MapWidget 类，用于显示交互式地图
#     * 内部依赖 Core 库和 Qt6::OpenGLWidgets（OpenGL 渲染后端）
#
#   - Qt6::Widgets：Qt Widgets 模块
#     * 提供 QWidget、QMainWindow 等基础 UI 类
#     * 是 Qt GUI 应用的基础依赖
#
#   - Qt6::OpenGLWidgets：Qt OpenGL Widgets 模块
#     * 提供 QOpenGLWidget 类
#     * MapLibre 使用 OpenGL 进行地图渲染，需要此模块
#
#   - Qt6::Network：Qt 网络模块
#     * 提供 QNetworkAccessManager、QTcpSocket 等网络功能
#     * 用于下载在线地图瓦片、访问 HXGISServer 服务等
#
target_link_libraries(appuntitled PRIVATE
    QMapLibre::Widgets
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Network
)

# =============================================================================
# Android 部署配置
# =============================================================================
#
# 【为什么需要额外配置】
#   Android 应用打包为 APK 格式，所有资源（库文件、清单、资源文件）都必须
#   在构建时明确指定，CMake 无法像 Linux 那样通过文件系统自动发现。
#
# 【QT_ANDROID_PACKAGE_SOURCE_DIR】
#   指定 Android 资源目录，该目录必须包含：
#     - AndroidManifest.xml：应用清单，定义包名、权限、Activity 等
#     - 可选：res/ 目录（图标、布局等）、src/ 目录（Java 代码）
#   Qt 构建系统会将此目录内容打包进 APK 的 assets。
#
# 【QT_ANDROID_EXTRA_LIBS】
#   指定需要打包进 APK 的额外共享库（.so 文件）。
#   默认情况下，Qt 只打包 Qt 自身的库和通过 target_link_libraries 链接的库。
#   HXGISServer 是手动通过 target_link_directories 链接的，Qt 不会自动检测，
#   因此需要显式指定将其打包进 APK 的 lib/ 目录。
#
# 【Linux/macOS/Windows 设置】
#   - MACOSX_BUNDLE TRUE：生成 macOS .app 应用包
#   - WIN32_EXECUTABLE TRUE：Windows GUI 子系统（无控制台窗口）
#
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

enable_testing()
add_subdirectory(tests)
