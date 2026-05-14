#include <QApplication>
#include <QWidget>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QPainter>
#include <QDebug>
#include "mapcontainer.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MapContainer::MapConfig config;
    config.styleUrl = "http://127.0.0.1:4943/styles/day/style.json?schema=hxmap";
    config.defaultCoordinate = QMapLibre::Coordinate(36.75, 3.05);
    config.defaultZoom = 8.0;

    MapContainer mapContainer(config);
    mapContainer.resize(800, 600);
    mapContainer.show();

    QTimer::singleShot(8000, &app, [&]() {
        qDebug() << "=== STEP 1: Registering icons ===";
        QMap<QString, QImage> icons;
        QImage redIcon(32, 32, QImage::Format_ARGB32);
        redIcon.fill(Qt::red);
        icons["marker"] = redIcon;

        QImage blueIcon(32, 32, QImage::Format_ARGB32);
        blueIcon.fill(Qt::blue);
        icons["flag"] = blueIcon;

        mapContainer.registerAnnotationIcons(icons);
        qDebug() << "Icons registered: marker (red), flag (blue)";

        qDebug() << "=== STEP 2: Adding multilingual annotations with icons ===";
        QVector<MapAnnotation> anns;

        // 3 annotations with icons - spread out to avoid text collision
        MapAnnotation a1;
        a1.id = "ann-zh-icon";
        a1.latitude = 36.78;
        a1.longitude = 3.04;
        a1.title = QString::fromUtf8("\xe4\xb8\xad\xe5\x9b\xbd"); // 中国
        a1.iconName = "marker";
        anns.append(a1);

        MapAnnotation a2;
        a2.id = "ann-en-icon";
        a2.latitude = 36.78;
        a2.longitude = 3.08;
        a2.title = "English";
        a2.iconName = "marker";
        anns.append(a2);

        MapAnnotation a3;
        a3.id = "ann-ru-icon";
        a3.latitude = 36.78;
        a3.longitude = 3.12;
        a3.title = QString::fromUtf8("\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9"); // Русский
        a3.iconName = "flag";
        anns.append(a3);

        // 3 text-only annotations - spread out below
        MapAnnotation a4;
        a4.id = "ann-zh-text";
        a4.latitude = 36.72;
        a4.longitude = 3.04;
        a4.title = QString::fromUtf8("\xe4\xb8\xad\xe6\x96\x87\xe6\xb5\x8b\xe8\xaf\x95"); // 中文测试
        a4.iconName = "";
        anns.append(a4);

        MapAnnotation a5;
        a5.id = "ann-en-text";
        a5.latitude = 36.72;
        a5.longitude = 3.08;
        a5.title = "TextOnly";
        a5.iconName = "";
        anns.append(a5);

        MapAnnotation a6;
        a6.id = "ann-ja-text";
        a6.latitude = 36.72;
        a6.longitude = 3.12;
        a6.title = QString::fromUtf8("\xe3\x81\x93\xe3\x82\x93\xe3\x81\xab\xe3\x81\xa1\xe3\x81\xaf"); // こんにちは
        a6.iconName = "";
        anns.append(a6);

        mapContainer.setAnnotations(anns);
        mapContainer.setCenter(36.75, 3.08);
        mapContainer.setZoom(13.0);

        qDebug() << "Annotations set, ids:" << mapContainer.allIds();
        qDebug() << "Visible ids:" << mapContainer.visibleIds();

        // Screenshot after render
        QTimer::singleShot(6000, &app, [&]() {
            qDebug() << "=== STEP 3: Taking screenshot ===";

            QPixmap pixmap = mapContainer.grab();
            QString path = "/home/guangbin/Documents/untitled/.sisyphus/evidence/multilingual-annotations.png";
            if (pixmap.save(path)) {
                qDebug() << "SUCCESS: Screenshot saved to" << path << "size:" << pixmap.size();
            } else {
                qDebug() << "FAILED: Could not save screenshot";
            }

            QScreen *screen = QGuiApplication::primaryScreen();
            if (screen) {
                QPixmap fullScreen = screen->grabWindow(0);
                fullScreen.save("/home/guangbin/Documents/untitled/.sisyphus/evidence/multilingual-fullscreen.png");
            }

            app.quit();
        });
    });

    return app.exec();
}
