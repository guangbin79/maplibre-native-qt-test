/**
 * @file main.cpp
 * @brief 应用程序入口 — 初始化本地 GIS 服务并启动 QWidget 地图界面
 *
 * 启动流程图（按执行顺序）：
 * ┌─────────────────────────────────────────────────────────────┐
 * │  1. 平台插件路径配置                                          │
 * │     ├─ Linux:  手动添加 SDK_PLUGINS_PATH（MapLibre 插件不在系统路径）│
 * │     └─ Android: Qt 自动处理，跳过                             │
 * │         ↓                                                   │
 * │  2. 创建 QApplication                                         │
 * │         ↓                                                   │
 * │  3. 确定 GIS 数据根目录                                        │
 * │     ├─ Android: AppDataLocation（应用专属数据目录）             │
 * │     └─ Linux:  可执行文件同目录下的 map_data/ 子目录           │
 * │         ↓                                                   │
 * │  4. 启动 HXGISServer（本地矢量瓦片服务）                       │
 * │     ├─ 构造时加载 libplugin-HXGISServer.so                    │
 * │     ├─ 启动本地 HTTP 服务（127.0.0.1:4943）                    │
 * │     ├─ 错误检查：端口占用、权限不足、库加载失败                 │
 * │     └─ 验证：server.isRunning()                               │
 * │         ↓                                                   │
 * │  5. 创建并显示 MainWindow                                     │
 * │     └─ QMainWindow + QOpenGLWidget + MapLibre 渲染            │
 * │         ↓                                                   │
 * │  6. 进入 Qt 事件循环 (QApplication::exec())                    │
 * └─────────────────────────────────────────────────────────────┘
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
     *
     * 为什么 Linux 需要手动设置？
     *   MapLibre Native Qt 插件（platforms/、imageformats/、geoservices/ 等）
     *   不会安装到 Linux 系统标准路径（如 /usr/lib/qt6/plugins/），而是随 SDK
     *   分发在独立的目录中。Qt 的插件加载机制通过 QCoreApplication::libraryPaths()
     *   查找插件，若路径中不包含 MapLibre 插件目录，运行时会出现如下错误：
     *     "Could not find the Qt platform plugin 'xcb'"
     *     "No geoservice provider for 'maplibre'"
     *   因此通过 CMake 在编译期注入 SDK_PLUGINS_PATH 宏，在启动时手动追加。
     *
     * 为什么 Android 不需要？
     *   Qt for Android 构建系统（androiddeployqt）会自动将所有插件打包到 APK 的
     *   assets 目录，并在 QApplication 构造时通过 Qt 内部机制自动设置库路径。
     *   手动添加反而可能导致路径重复或权限问题。
     */
#ifndef IS_ANDROID
#ifdef SDK_PLUGINS_PATH
    QCoreApplication::addLibraryPath(SDK_PLUGINS_PATH);
#endif
#endif

    QApplication app(argc, argv);

    /* ── 2. 确定 GIS 数据根目录 ────────────────────────────────
     *
     * GIS Server 需要一个 root_path 作为瓦片数据的存放/查找根目录。
     * 不同平台采用不同策略，原因如下：
     *
     * Android - 使用 QStandardPaths::AppDataLocation：
     *   - 原因：Android 应用只能写入应用专属目录（沙箱机制），无法访问
     *     外部存储（如 /sdcard/）除非申请额外权限。
     *   - AppDataLocation 返回的路径示例：
     *     /data/data/<package_name>/files/
     *     其中 <package_name> 如 org.qtproject.example.untitled
     *   - 使用 mkpath() 确保目录存在，因为首次安装时该目录可能尚未创建。
     *
     * Linux - 使用可执行文件同目录下的 map_data/ 子目录：
     *   - 原因：桌面开发/调试场景下，数据文件通常与可执行文件一起分发，
     *     便于版本管理和快速迭代。生产环境可通过安装脚本将数据部署到此处。
     *   - 路径示例：/home/user/project/build/linux-x86_64/map_data/
     *   - 通过 QCoreApplication::applicationDirPath() 动态获取，确保无论
     *     从何处启动都能找到相对路径的数据目录。
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
     *
     * HXGISServer 是对 libplugin-HXGISServer.so 的 C++ 封装。
     * 构造时调用 C API plugin_HXGISServer_create() 启动本地 HTTP 服务，
     * 提供 Vector Tiles (MVT) 和 MapLibre Style JSON。
     *
     * 服务地址：http://127.0.0.1:4943
     *   - 127.0.0.1 表示仅本地可访问，避免外部网络暴露
     *   - 端口 4943 为项目约定端口（非标准 HTTP 80/8080，避免冲突）
     *
     * 常见启动失败场景及排查：
     *   1. 端口占用：4943 被其他进程占用（如上次崩溃未释放）
     *      → 检查：lsof -i :4943 或 netstat -tlnp | grep 4943
     *   2. 权限不足：非 root 用户尝试绑定特权端口（<1024）
     *      → 当前使用 4943（>1024），一般不会出现
     *   3. 库加载失败：libplugin-HXGISServer.so 不在 LD_LIBRARY_PATH 或 rpath 中
     *      → 检查：ldd <可执行文件> | grep HXGISServer
     *   4. 数据目录无效：rootPath 不存在或无读写权限
     *      → 检查：确认 map_data/ 目录存在且包含有效的瓦片数据
     *
     * 验证方式：
     *   - 代码层面：server.isRunning() 返回 true/false
     *   - 手动验证：curl http://127.0.0.1:4943/styles/day/style.json
     *   - 日志输出：server.version() 返回版本字符串表示初始化成功
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
     *
     * MainWindow 继承自 QMainWindow，是应用程序的主窗口。
     * 内部架构：
     *   - QOpenGLWidget: 作为 MapLibre Native 的渲染表面，提供硬件加速的
     *     OpenGL 上下文。QOpenGLWidget 自动管理 GL 上下文的创建、销毁和
     *     线程安全，无需手动调用 makeCurrent()/doneCurrent()。
     *   - MapLibre 集成: 通过 QMapLibre::Map 对象与 QOpenGLWidget 关联，
     *     从 HXGISServer (http://127.0.0.1:4943) 加载矢量瓦片和样式。
     *
     * 为什么使用 QOpenGLWidget 而非 QWidget + QPainter：
     *   - MapLibre Native 基于 OpenGL ES 2.0/3.0 渲染，需要原生 GL 表面
     *   - QPainter 的栅格绘制无法满足实时矢量地图的性能要求
     *   - QOpenGLWidget 支持跨平台（Linux X11/Wayland、Android SurfaceFlinger）
     *
     * 窗口生命周期：
     *   - show() 后进入事件循环，窗口关闭时自动触发 QApplication::quit()
     *   - MainWindow 析构时会自动清理 MapLibre 资源和 OpenGL 上下文
     */
    MainWindow window;
#ifdef IS_ANDROID
    window.showFullScreen();
#else
    window.show();
#endif

    return app.exec();
}
