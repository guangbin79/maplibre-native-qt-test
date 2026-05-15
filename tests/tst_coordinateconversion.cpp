/**
 * @file tst_coordinateconversion.cpp
 * @brief Coordinate conversion unit test for MapContainer
 *
 * Tests coordinateToScreen() and screenToCoordinate() methods.
 */

#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include <QDebug>

#include "mapcontainer.h"
#include "hxgisserver.h"

class tst_CoordinateConversion : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testCoordinateToScreen();
    void testScreenToCoordinate();
    void testRoundTrip();
    void cleanupTestCase();

private:
    MapContainer *m_map = nullptr;
    HXGISServer *m_server = nullptr;
};

void tst_CoordinateConversion::initTestCase()
{
    QString rootPath = "/home/guangbin/Documents/untitled/build/linux-x86_64/map_data";
    m_server = new HXGISServer("127.0.0.1:4943", rootPath.toUtf8().constData());
    QVERIFY2(m_server->isRunning(), "Failed to start HXGISServer");

    QTest::qWait(500);

    MapContainer::MapConfig config;
    config.styleUrl = "http://127.0.0.1:4943/styles/day/style.json?schema=hxmap";
    config.defaultCoordinate = QMapLibre::Coordinate(39.9, 116.4);
    config.defaultZoom = 10.0;

    m_map = new MapContainer(config);
    m_map->resize(800, 600);
    m_map->show();

    QCoreApplication::processEvents();
    QTest::qWait(500);

    if (!m_map->isMapReady()) {
        QSignalSpy readySpy(m_map, &MapContainer::mapReady);
        readySpy.wait(20000);
    }

    QVERIFY(m_map->isMapReady());
}

void tst_CoordinateConversion::testCoordinateToScreen()
{
    QPointF pixel = m_map->coordinateToScreen(39.9, 116.4);

    // Map center should be near viewport center
    QPointF viewportCenter(m_map->width() / 2.0, m_map->height() / 2.0);

    QVERIFY2(qAbs(pixel.x() - viewportCenter.x()) < 5.0,
             qPrintable(QString("X off: got %1, expected ~%2").arg(pixel.x()).arg(viewportCenter.x())));
    QVERIFY2(qAbs(pixel.y() - viewportCenter.y()) < 5.0,
             qPrintable(QString("Y off: got %1, expected ~%2").arg(pixel.y()).arg(viewportCenter.y())));
}

void tst_CoordinateConversion::testScreenToCoordinate()
{
    QPointF viewportCenter(m_map->width() / 2.0, m_map->height() / 2.0);
    QMapLibre::Coordinate coord = m_map->screenToCoordinate(viewportCenter);

    QVERIFY2(qAbs(coord.first - 39.9) < 0.01,
             qPrintable(QString("Lat off: got %1, expected ~39.9").arg(coord.first)));
    QVERIFY2(qAbs(coord.second - 116.4) < 0.01,
             qPrintable(QString("Lon off: got %1, expected ~116.4").arg(coord.second)));
}

void tst_CoordinateConversion::testRoundTrip()
{
    double lat = 31.2304, lon = 121.4737; // Shanghai

    QPointF pixel = m_map->coordinateToScreen(lat, lon);
    QMapLibre::Coordinate result = m_map->screenToCoordinate(pixel);

    QVERIFY2(qAbs(result.first - lat) < 0.001,
             qPrintable(QString("Round-trip lat delta too large: got %1, expected %2").arg(result.first).arg(lat)));
    QVERIFY2(qAbs(result.second - lon) < 0.001,
             qPrintable(QString("Round-trip lon delta too large: got %1, expected %2").arg(result.second).arg(lon)));
}

void tst_CoordinateConversion::cleanupTestCase()
{
    delete m_map;
    m_map = nullptr;
    delete m_server;
    m_server = nullptr;
}

QTEST_MAIN(tst_CoordinateConversion)
#include "tst_coordinateconversion.moc"
