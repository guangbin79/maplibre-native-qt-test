/**
 * @file main.cpp
 * @brief 应用程序入口 — 初始化本地 GIS 服务并启动 MapLibre 地图界面
 *
 * 整体流程：
 *   1. 根据平台（Linux / Android）配置 Qt 插件路径与数据目录
 *   2. 强制使用 OpenGL 渲染后端（MapLibre 依赖 OpenGL）
 *   3. 启动 HXGISServer（本地矢量瓦片服务，监听 127.0.0.1:4943）
 *   4. 创建 QML 引擎，加载 Main.qml 地图界面
 *
 * 依赖：
 *   - Qt 6.6+ (Quick, Positioning, Location)
 *   - MapLibre Native Qt 绑定 (QMapLibre)
 *   - libplugin-HXGISServer.so（闭源本地瓦片服务库）
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QQuickWindow>
#include <QDir>
#include <QStandardPaths>

#include "hxgisserver.h"

int main(int argc, char *argv[])
{
    /* ── 1. 平台相关插件路径 ────────────────────────────────────
     * Linux 下 MapLibre 的 Qt 插件（platforms、imageformats 等）
     * 不在系统默认路径，需要通过 CMake 注入的 SDK_PLUGINS_PATH 宏手动添加。
     * Android 下 Qt for Android 会自动处理插件路径，无需手动设置。
     */
#ifndef IS_ANDROID
#ifdef SDK_PLUGINS_PATH
    QCoreApplication::addLibraryPath(SDK_PLUGINS_PATH);
#endif
#endif

    /* ── 2. 强制 OpenGL 渲染后端 ───────────────────────────────
     * MapLibre Native 依赖 OpenGL 进行矢量瓦片渲染。
     * 在 Qt 6 中默认可能使用 Vulkan 或软件渲染，必须显式指定 OpenGL RHI。
     */
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);

    QGuiApplication app(argc, argv);

    /* ── 3. 确定 GIS 数据根目录 ────────────────────────────────
     * GIS Server 需要一个 root_path 作为瓦片数据的存放/查找根目录：
     *   - Android: 使用应用专属数据目录 (AppDataLocation)
     *              例如 /data/data/org.qtproject.example.untitled/files/
     *   - Linux:   使用可执行文件同目录下的 map_data/ 子目录
     *              例如 /path/to/build/linux-x86_64/map_data/
     */
#ifdef IS_ANDROID
    QString rootPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(rootPath);  // 确保目录存在（Android 首次运行时可能不存在）
#else
    QString rootPath = QCoreApplication::applicationDirPath() + "/map_data";
#endif

    // GIS Server 固定参数：本地监听地址和端口
    static const char *gisUrl = "127.0.0.1:4943";

    qDebug() << "HXGIS root path:" << rootPath;

    /* ── 4. 启动 HXGIS Server ──────────────────────────────────
     * HXGISServer 是对 libplugin-HXGISServer.so 的 C++ 封装。
     * 构造时调用 C API plugin_HXGISServer_create() 启动本地 HTTP 服务，
     * 提供 Vector Tiles (MVT) 和 MapLibre Style JSON。
     *
     * 启动后可通过 http://127.0.0.1:4943/styles/day/style.json 访问地图样式。
     */
    HXGISServer server(gisUrl, rootPath.toUtf8().constData());
    if (!server.isRunning()) {
        qCritical() << "Failed to start HXGIS Server on" << gisUrl;
        return 1;
    }
    qDebug() << "HXGIS Server started, version:" << server.version()
             << "root_path:" << rootPath;

    /* ── 5. 创建 QML 引擎 ──────────────────────────────────────
     * Linux 下需要手动添加 MapLibre QML 模块的导入路径，
     * Android 下 Qt for Android 会自动发现 QML 模块。
     */
    QQmlApplicationEngine engine;

#ifndef IS_ANDROID
#ifdef SDK_QML_PATH
    engine.addImportPath(SDK_QML_PATH);
#endif
#endif

    // 打印导入路径和可用插件（调试用）
    qDebug() << "Import paths:" << engine.importPathList();

    // 从 QML 模块 "untitled" 中加载 Main.qml（由 CMakeLists.txt 的 qt_add_qml_module 定义）
    engine.loadFromModule("untitled", "Main");

    return QCoreApplication::exec();
}
