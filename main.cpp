#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QQuickWindow>

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
