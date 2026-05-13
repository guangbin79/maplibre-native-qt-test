#include <QtTest>
#include "mappolygon.h"

class MapPolygonTest : public QObject {
    Q_OBJECT

private slots:
    void testDefaultValues()
    {
        MapPolygon poly;
        QCOMPARE(poly.id, QString());
        QCOMPARE(poly.polygonId, QString());
        QCOMPARE(poly.coordinates.size(), 0);
        QCOMPARE(poly.fillEnabled, true);
        QVERIFY(!poly.fillColor.isValid());
        QCOMPARE(poly.fillOpacity, 0.5);
        QVERIFY(!poly.strokeColor.isValid());
        QCOMPARE(poly.strokeWidth, 2.0);
        QCOMPARE(poly.strokeDashed, false);
        QCOMPARE(poly.title, QString());
        QVERIFY(!poly.isValid());
    }

    void testValidPolygon()
    {
        MapPolygon poly;
        poly.id = QStringLiteral("poly-001");
        poly.polygonId = QStringLiteral("zone-A");
        poly.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}, {39.9300, 116.3900}};
        poly.fillEnabled = true;
        poly.fillColor = QColor(255, 0, 0);
        poly.fillOpacity = 0.5;
        poly.strokeColor = QColor(0, 0, 0);
        poly.strokeWidth = 2.0;
        poly.strokeDashed = false;
        poly.title = QStringLiteral("区域A");
        QVERIFY(poly.isValid());
    }

    void testInvalidEmptyId()
    {
        MapPolygon poly;
        poly.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}, {39.9300, 116.3900}};
        poly.fillColor = QColor(0, 0, 255);
        poly.strokeColor = QColor(0, 128, 0);
        QVERIFY(!poly.isValid());
    }

    void testInvalidCoordinatesTooFew()
    {
        MapPolygon poly;
        poly.id = QStringLiteral("poly-few");
        poly.fillColor = QColor(0, 128, 0);
        poly.strokeColor = QColor(0, 0, 0);
        poly.coordinates = {};
        QVERIFY(!poly.isValid());
        poly.coordinates = {{39.9, 116.4}};
        QVERIFY(!poly.isValid());
        poly.coordinates = {{39.9, 116.4}, {31.2, 121.5}};
        QVERIFY(!poly.isValid());
    }

    void testInvalidLatitude()
    {
        MapPolygon poly;
        poly.id = QStringLiteral("poly-lat");
        poly.fillColor = QColor(255, 255, 0);
        poly.strokeColor = QColor(0, 0, 0);
        poly.coordinates = {{0.0, 0.0}, {91.0, 0.0}, {0.0, 10.0}};
        QVERIFY(!poly.isValid());
        poly.coordinates = {{0.0, 0.0}, {-91.0, 0.0}, {0.0, 10.0}};
        QVERIFY(!poly.isValid());
    }

    void testInvalidLongitude()
    {
        MapPolygon poly;
        poly.id = QStringLiteral("poly-lon");
        poly.fillColor = QColor(0, 255, 255);
        poly.strokeColor = QColor(0, 0, 0);
        poly.coordinates = {{0.0, 0.0}, {0.0, 181.0}, {10.0, 0.0}};
        QVERIFY(!poly.isValid());
        poly.coordinates = {{0.0, 0.0}, {0.0, -181.0}, {10.0, 0.0}};
        QVERIFY(!poly.isValid());
    }

    void testBoundaryValues()
    {
        MapPolygon poly;
        poly.id = QStringLiteral("poly-boundary");
        poly.fillColor = QColor(100, 100, 100);
        poly.strokeColor = QColor(0, 0, 0);
        poly.coordinates = {{90.0, 180.0}, {-90.0, -180.0}, {0.0, 0.0}};
        QVERIFY(poly.isValid());
        poly.coordinates = {{90.0, -180.0}, {-90.0, 180.0}, {0.0, 0.0}};
        QVERIFY(poly.isValid());
        poly.coordinates = {{-90.0, -180.0}, {0.0, 0.0}, {90.0, 180.0}};
        QVERIFY(poly.isValid());
    }

    void testTriangle()
    {
        MapPolygon poly;
        poly.id = QStringLiteral("poly-triangle");
        poly.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}, {39.9300, 116.3900}};
        QVERIFY(poly.isValid());
    }
};

QTEST_MAIN(MapPolygonTest)
#include "tst_mappolygon.moc"
