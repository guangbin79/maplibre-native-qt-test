#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <QCommandLineOption>

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

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption rootPathOption("root-path", "Map data root directory", "path", "/tmp/hx_gis_test");
    QCommandLineOption portOption("port", "HTTP server port", "port", "8080");
    parser.addOption(rootPathOption);
    parser.addOption(portOption);
    parser.process(app);

    QString rootPath = parser.value(rootPathOption);
    QString port = parser.value(portOption);
    QString url = QString("0.0.0.0:%1").arg(port);

    HXGISServer server(url.toUtf8().constData(), rootPath.toUtf8().constData());
    if (!server.isRunning()) {
        qCritical() << "Failed to start HXGIS Server on" << url;
        return 1;
    }
    qDebug() << "HXGIS Server started, version:" << server.version();

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
