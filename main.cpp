#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QQuickWindow>
#include <QDir>
#include <QStandardPaths>

#include "hxgisserver.h"

int main(int argc, char *argv[])
{
// 只有非 Android 平台（如 Linux）才需要手动指定插件路径
#ifndef IS_ANDROID
#ifdef SDK_PLUGINS_PATH
    // 使用 CMake 传进来的宏路径
    QCoreApplication::addLibraryPath(SDK_PLUGINS_PATH);
#endif
#endif

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);

    QGuiApplication app(argc, argv);

#ifdef IS_ANDROID
    // Android: HXGISServer 使用应用内部存储路径作为 root_path
    // map_data 由运行时按需获取或使用在线数据源
    QString rootPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(rootPath);
#else
    // Linux: map_data 位于可执行文件同目录
    QString rootPath = QCoreApplication::applicationDirPath() + "/map_data";
#endif

    // GIS Server 固定参数
    static const char *gisUrl = "127.0.0.1:4943";

    qDebug() << "HXGIS root path:" << rootPath;

    HXGISServer server(gisUrl, rootPath.toUtf8().constData());
    if (!server.isRunning()) {
        qCritical() << "Failed to start HXGIS Server on" << gisUrl;
        return 1;
    }
    qDebug() << "HXGIS Server started, version:" << server.version()
             << "root_path:" << rootPath;

    QQmlApplicationEngine engine;

#ifndef IS_ANDROID
#ifdef SDK_QML_PATH
    // 添加 MapLibre QML 模块路径
    engine.addImportPath(SDK_QML_PATH);
#endif
#endif

    // 打印导入路径和可用插件
    qDebug() << "Import paths:" << engine.importPathList();

    engine.loadFromModule("untitled", "Main");

    return QCoreApplication::exec();
}
