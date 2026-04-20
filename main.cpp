/**
 * @file main.cpp
 * @brief 应用程序入口 — 初始化本地 GIS 服务并启动 QWidget 地图界面
 *
 * 整体流程：
 *   1. 根据平台（Linux / Android）配置 Qt 插件路径与数据目录
 *   2. 启动 HXGISServer（本地矢量瓦片服务，监听 127.0.0.1:4943）
 *   3. 创建 MainWindow（QMainWindow），内含 QOpenGLWidget + MapLibre 渲染
 *
 * 依赖：
 *   - Qt 6.6+ (Widgets, OpenGL)
 *   - MapLibre Native Qt 绑定 (QMapLibre)
 *   - libplugin-HXGISServer.so（闭源本地瓦片服务库）
 */

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "hxgisserver.h"
#include "mainwindow.h"

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

    QApplication app(argc, argv);

    /* ── 2. 确定 GIS 数据根目录 ────────────────────────────────
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

    /* ── 3. 启动 HXGIS Server ──────────────────────────────────
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

    /* ── 4. 创建 MainWindow ─────────────────────────────────────
     * MainWindow 是 QMainWindow 子类，内含 QOpenGLWidget 承载 MapLibre 渲染。
     * QOpenGLWidget 自动管理 OpenGL 上下文，无需手动设置 RHI 后端。
     */
    MainWindow window;
    window.show();

    return app.exec();
}
