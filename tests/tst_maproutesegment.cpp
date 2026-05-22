#include <QtTest>
#include "maproutesegment.h"

class MapRouteSegmentTest : public QObject {
    Q_OBJECT

private slots:
    void testDefaultValues()
    {
        MapRouteSegment seg;
        QCOMPARE(seg.id, QString());
        QCOMPARE(seg.routeId, QString());
        QCOMPARE(seg.coordinates.size(), 0);
        QVERIFY(!seg.color.isValid());
        QCOMPARE(seg.width, 0.0);
        QCOMPARE(seg.dashed, false);
        QCOMPARE(seg.title, QString());
        QVERIFY(!seg.isValid());
    }

    void testValidSegment()
    {
        MapRouteSegment seg;
        seg.id = QStringLiteral("seg-001");
        seg.routeId = QStringLiteral("route-A");
        seg.coordinates = {{39.9042, 116.4074}, {31.2304, 121.4737}};
        seg.color = QColor(255, 0, 0);
        seg.width = 3.0;
        seg.dashed = false;
        seg.title = QStringLiteral("北京到上海");
        QVERIFY(seg.isValid());
    }

    void testInvalidEmptyId()
    {
        MapRouteSegment seg;
        seg.coordinates = {{39.9, 116.4}, {31.2, 121.5}};
        seg.color = QColor(0, 0, 255);
        seg.width = 2.0;
        QVERIFY(!seg.isValid());
    }

    void testInvalidCoordinatesTooFew()
    {
        MapRouteSegment seg;
        seg.id = QStringLiteral("seg-few");
        seg.color = QColor(0, 128, 0);
        seg.width = 1.5;
        seg.coordinates = {};
        QVERIFY(!seg.isValid());
        seg.coordinates = {{39.9, 116.4}};
        QVERIFY(!seg.isValid());
    }

    void testInvalidLatitude()
    {
        MapRouteSegment seg;
        seg.id = QStringLiteral("seg-lat");
        seg.color = QColor(255, 255, 0);
        seg.width = 2.0;
        seg.coordinates = {{0.0, 0.0}, {91.0, 0.0}};
        QVERIFY(!seg.isValid());
        seg.coordinates = {{0.0, 0.0}, {-91.0, 0.0}};
        QVERIFY(!seg.isValid());
    }

    void testInvalidLongitude()
    {
        MapRouteSegment seg;
        seg.id = QStringLiteral("seg-lon");
        seg.color = QColor(0, 255, 255);
        seg.width = 2.0;
        seg.coordinates = {{0.0, 0.0}, {0.0, 181.0}};
        QVERIFY(!seg.isValid());
        seg.coordinates = {{0.0, 0.0}, {0.0, -181.0}};
        QVERIFY(!seg.isValid());
    }

    void testInvalidDefaultColor()
    {
        MapRouteSegment seg;
        seg.id = QStringLiteral("seg-color");
        seg.coordinates = {{39.9, 116.4}, {31.2, 121.5}};
        seg.width = 2.0;
        QVERIFY(!seg.color.isValid());
        QVERIFY(!seg.isValid());
    }

    void testInvalidWidth()
    {
        MapRouteSegment seg;
        seg.id = QStringLiteral("seg-width");
        seg.coordinates = {{39.9, 116.4}, {31.2, 121.5}};
        seg.color = QColor(128, 0, 128);
        seg.width = 0.0;
        QVERIFY(!seg.isValid());
        seg.width = -1.0;
        QVERIFY(!seg.isValid());
    }

    void testBoundaryValues()
    {
        MapRouteSegment seg;
        seg.id = QStringLiteral("seg-boundary");
        seg.color = QColor(100, 100, 100);
        seg.width = 1.0;
        seg.coordinates = {{90.0, 180.0}, {-90.0, -180.0}};
        QVERIFY(seg.isValid());
        seg.coordinates = {{90.0, -180.0}, {-90.0, 180.0}};
        QVERIFY(seg.isValid());
        seg.coordinates = {{-90.0, -180.0}, {0.0, 0.0}};
        QVERIFY(seg.isValid());
    }

    void testEmptyTitle()
    {
        MapRouteSegment seg;
        seg.id = QStringLiteral("seg-notitle");
        seg.coordinates = {{39.9, 116.4}, {31.2, 121.5}};
        seg.color = QColor(0, 128, 255);
        seg.width = 2.0;
        QVERIFY(seg.title.isEmpty());
        QVERIFY(seg.isValid());
    }

    void testEmptyRouteId()
    {
        MapRouteSegment seg;
        seg.id = QStringLiteral("seg-noroute");
        seg.coordinates = {{39.9, 116.4}, {31.2, 121.5}};
        seg.color = QColor(200, 100, 0);
        seg.width = 2.5;
        QVERIFY(seg.routeId.isEmpty());
        QVERIFY(seg.isValid());
    }

    void showArrows_default_is_false()
    {
        MapRouteSegment seg;
        QCOMPARE(seg.showArrows, false);
    }

    void arrowSize_default_is_1()
    {
        MapRouteSegment seg;
        QCOMPARE(seg.arrowSize, 1.0);
    }

    void arrowColor_default_invalid()
    {
        MapRouteSegment seg;
        QVERIFY(!seg.arrowColor.isValid());
    }

    void effectiveArrowColor_fallback_to_color()
    {
        MapRouteSegment seg;
        seg.color = QColor(255, 0, 0);
        QVERIFY(!seg.arrowColor.isValid());
        QColor arrowColor = seg.effectiveArrowColor();
        QCOMPARE(arrowColor.toRgb(), QColor(0, 0, 0));
    }

    void effectiveArrowColor_uses_custom()
    {
        MapRouteSegment seg;
        seg.color = QColor(255, 0, 0);
        seg.arrowColor = QColor(0, 0, 255);
        QCOMPARE(seg.effectiveArrowColor(), QColor(0, 0, 255));
    }

    void isValid_unchanged_by_arrow_fields()
    {
        MapRouteSegment seg;
        seg.id = QStringLiteral("seg-arrow");
        seg.coordinates = {{39.9, 116.4}, {31.2, 121.5}};
        seg.color = QColor(0, 128, 0);
        seg.width = 2.0;
        QVERIFY(seg.isValid());

        seg.showArrows = true;
        seg.arrowSize = 2.0;
        seg.arrowColor = QColor("#FFFFFF");
        QVERIFY(seg.isValid());

        seg.arrowColor = QColor();
        QVERIFY(seg.isValid());
    }
};

QTEST_MAIN(MapRouteSegmentTest)
#include "tst_maproutesegment.moc"
