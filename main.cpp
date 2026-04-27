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
     * │     ├─ Android: 外部存储 /sdcard/map_data（需授权）             │
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
#include <QMessageBox>
#include <QStandardPaths>

#include "hxgisserver.h"
#include "mainwindow.h"

#ifdef IS_ANDROID
#include <QJniObject>
#include <QNativeInterface/QAndroidApplication>

/**
 * @brief 检查是否已获取 MANAGE_EXTERNAL_STORAGE 权限
 *
 * 通过调用 android.os.Environment.isExternalStorageManager() 判断。
 * 该权限允许应用访问设备上所有文件（包括 SD 卡任意位置），
 * 适用于企业内部分发应用（非 Google Play 渠道）。
 */
bool hasManageExternalStoragePermission()
{
    QJniObject envClass("android/os/Environment");
    if (!envClass.isValid())
        return false;
    jboolean isManager = envClass.callStaticMethod<jboolean>(
        "isExternalStorageManager",
        "()Z"
    );
    return static_cast<bool>(isManager);
}

/**
 * @brief 跳转到系统设置页请求 MANAGE_EXTERNAL_STORAGE 权限
 *
 * 启动 android.settings.MANAGE_APP_ALL_FILES_ACCESS_PERMISSION Intent，
 * 将用户带到"所有文件访问权限"设置页面。用户授权后返回应用，
 * 下次启动即可检测到权限已授予。
 */
void requestManageExternalStoragePermission()
{
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    QJniObject packageName = context.callObjectMethod(
        "getPackageName",
        "()Ljava/lang/String;"
    );

    QJniObject intent(
        "android/content/Intent",
        "(Ljava/lang/String;)V",
        QJniObject::fromString(
            "android.settings.MANAGE_APP_ALL_FILES_ACCESS_PERMISSION"
        ).object<jstring>()
    );

    QJniObject uriString = QJniObject::fromString(
        "package:" + packageName.toString()
    );
    QJniObject uri = QJniObject::callStaticObjectMethod(
        "android/net/Uri",
        "parse",
        "(Ljava/lang/String;)Landroid/net/Uri;",
        uriString.object<jstring>()
    );

    intent.callObjectMethod(
        "setData",
        "(Landroid/net/Uri;)Landroid/content/Intent;",
        uri.object<jobject>()
    );

    context.callMethod<void>(
        "startActivity",
        "(Landroid/content/Intent;)V",
        intent.object<jobject>()
    );
}

/**
 * @brief 获取外部存储上的 GIS 数据根目录
 *
 * 优先检查 /sdcard/map_data，其次 /storage/emulated/0/map_data。
 * 若目录已存在则直接返回；否则返回 /sdcard/map_data（用户需自行放置数据）。
 */
QString getExternalStorageRootPath()
{
    QStringList candidates = {
        QStringLiteral("/sdcard/map_data"),
        QStringLiteral("/storage/emulated/0/map_data")
    };

    for (const QString &path : candidates) {
        if (QDir(path).exists())
            return path;
    }

    return QStringLiteral("/sdcard/map_data");
}
#endif

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
     * Android - 始终使用外部存储 /sdcard/map_data：
     *   - 通过 MANAGE_EXTERNAL_STORAGE 权限访问 SD 卡任意位置，便于用户
     *     直接拷贝地图数据到设备而无需通过 ADB push 到应用私有目录。
     *   - 首次启动若未授权，自动跳转系统设置页引导用户授权，
     *     始终不切换至 AppDataLocation，确保数据路径统一。
     *
     * Linux - 使用可执行文件同目录下的 map_data/ 子目录：
     *   - 原因：桌面开发/调试场景下，数据文件通常与可执行文件一起分发，
     *     便于版本管理和快速迭代。生产环境可通过安装脚本将数据部署到此处。
     *   - 路径示例：/home/user/project/build/linux-x86_64/map_data/
     *   - 通过 QCoreApplication::applicationDirPath() 动态获取，确保无论
     *     从何处启动都能找到相对路径的数据目录。
     */
#ifdef IS_ANDROID
    if (!hasManageExternalStoragePermission()) {
        requestManageExternalStoragePermission();
        QMessageBox::warning(nullptr, QStringLiteral("需要存储权限"),
            QStringLiteral("本应用需要访问外部存储权限才能读取地图数据。\n\n"
                           "请在系统设置中授予\"所有文件访问权限\"，"
                           "然后重新启动应用。"));
        return 1;
    }

    QString rootPath = getExternalStorageRootPath();

    if (!QDir(rootPath).exists()) {
        QMessageBox::warning(nullptr, QStringLiteral("缺少地图数据"),
            QStringLiteral("未找到地图数据目录：%1\n\n"
                           "请在手机存储根目录创建 map_data 文件夹，"
                           "并将地图数据拷贝到该目录后重启应用。")
                .arg(rootPath));
        return 1;
    }

    qDebug() << "Android root path (external storage):" << rootPath;
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
