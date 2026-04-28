#include <QtTest/QtTest>
#include "routemanager.h"

class TestRouteManager : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testSetSegments();
    void testAddRouteSegment();
    void testAddRouteSegmentsBatch();
    void testRemoveRouteSegment();
    void testRemoveRouteSegmentsBatch();
    void testSetVisibleRouteIds();
    void testShowAllRoutes();
    void testHideAllRoutes();
    void testClearSegments();
    void testMapNotReady();
    void testAllRouteIdsDedup();
};

static MapRouteSegment makeSegment(const QString& id, const QString& routeId,
                                    const QString& title = "", bool dashed = false)
{
    MapRouteSegment seg;
    seg.id = id;
    seg.routeId = routeId;
    seg.coordinates = {{39.9, 116.4}, {31.2, 121.5}};
    seg.color = QColor("#ff0000");
    seg.width = 3.0;
    seg.dashed = dashed;
    seg.title = title;
    return seg;
}

void TestRouteManager::testInitialState()
{
    RouteManager mgr(nullptr);
    QVERIFY(mgr.allRouteIds().isEmpty());
    QVERIFY(mgr.visibleRouteIds().isEmpty());
}

void TestRouteManager::testSetSegments()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapRouteSegment> segs;
    segs.append(makeSegment("s1", "routeA", "Route A"));
    segs.append(makeSegment("s2", "routeB", "Route B"));

    mgr.setSegments(segs);

    QStringList ids = mgr.allRouteIds();
    QCOMPARE(ids.size(), 2);
    QVERIFY(ids.contains("routeA"));
    QVERIFY(ids.contains("routeB"));
    QCOMPARE(mgr.visibleRouteIds().size(), 2);
}

void TestRouteManager::testAddRouteSegment()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeSegment("s1", "routeA", "Route A"));

    QStringList ids = mgr.allRouteIds();
    QCOMPARE(ids.size(), 1);
    QCOMPARE(ids.first(), QString("routeA"));
    QCOMPARE(mgr.visibleRouteIds().size(), 1);
}

void TestRouteManager::testAddRouteSegmentsBatch()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapRouteSegment> segs;
    segs.append(makeSegment("s1", "routeA", "Route A"));
    segs.append(makeSegment("s2", "routeB", "Route B"));

    mgr.addRouteSegments(segs);

    QStringList ids = mgr.allRouteIds();
    QCOMPARE(ids.size(), 2);
    QVERIFY(ids.contains("routeA"));
    QVERIFY(ids.contains("routeB"));
    QCOMPARE(mgr.visibleRouteIds().size(), 2);
}

void TestRouteManager::testRemoveRouteSegment()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeSegment("s1", "routeA", "Route A"));
    QCOMPARE(mgr.allRouteIds().size(), 1);

    mgr.removeRouteSegment("s1");
    QVERIFY(mgr.allRouteIds().isEmpty());
    QVERIFY(mgr.visibleRouteIds().isEmpty());
}

void TestRouteManager::testRemoveRouteSegmentsBatch()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapRouteSegment> segs;
    segs.append(makeSegment("s1", "routeA", "Route A"));
    segs.append(makeSegment("s2", "routeB", "Route B"));
    segs.append(makeSegment("s3", "routeC", "Route C"));

    mgr.setSegments(segs);
    QCOMPARE(mgr.allRouteIds().size(), 3);

    mgr.removeRouteSegments({"s1", "s2"});
    QStringList ids = mgr.allRouteIds();
    QCOMPARE(ids.size(), 1);
    QCOMPARE(ids.first(), QString("routeC"));
}

void TestRouteManager::testSetVisibleRouteIds()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapRouteSegment> segs;
    segs.append(makeSegment("s1", "routeA", "Route A"));
    segs.append(makeSegment("s2", "routeB", "Route B"));

    mgr.setSegments(segs);
    QCOMPARE(mgr.visibleRouteIds().size(), 2);

    mgr.setVisibleRouteIds({"routeA"});
    QCOMPARE(mgr.visibleRouteIds().size(), 1);
    QCOMPARE(mgr.visibleRouteIds().first(), QString("routeA"));
}

void TestRouteManager::testShowAllRoutes()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapRouteSegment> segs;
    segs.append(makeSegment("s1", "routeA", "Route A"));
    segs.append(makeSegment("s2", "routeB", "Route B"));

    mgr.setSegments(segs);
    mgr.hideAllRoutes();
    QVERIFY(mgr.visibleRouteIds().isEmpty());

    mgr.showAllRoutes();
    QCOMPARE(mgr.visibleRouteIds().size(), 2);
}

void TestRouteManager::testHideAllRoutes()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeSegment("s1", "routeA", "Route A"));
    QCOMPARE(mgr.visibleRouteIds().size(), 1);

    mgr.hideAllRoutes();
    QVERIFY(mgr.visibleRouteIds().isEmpty());
}

void TestRouteManager::testClearSegments()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeSegment("s1", "routeA", "Route A"));
    QCOMPARE(mgr.allRouteIds().size(), 1);

    mgr.clearSegments();
    QVERIFY(mgr.allRouteIds().isEmpty());
    QVERIFY(mgr.visibleRouteIds().isEmpty());
}

void TestRouteManager::testMapNotReady()
{
    RouteManager mgr(nullptr);

    mgr.addRouteSegment(makeSegment("s1", "routeA", "Route A"));
    QVERIFY(mgr.allRouteIds().isEmpty());

    mgr.setSegments({makeSegment("s1", "routeA", "Route A")});
    QVERIFY(mgr.allRouteIds().isEmpty());
}

void TestRouteManager::testAllRouteIdsDedup()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapRouteSegment> segs;
    segs.append(makeSegment("s1", "routeA", "Route A"));
    segs.append(makeSegment("s2", "routeA", "Route A"));
    segs.append(makeSegment("s3", "routeB", "Route B"));

    mgr.setSegments(segs);
    QStringList ids = mgr.allRouteIds();
    QCOMPARE(ids.size(), 2);
    QVERIFY(ids.contains("routeA"));
    QVERIFY(ids.contains("routeB"));
}

QTEST_MAIN(TestRouteManager)
#include "tst_routemanager.moc"
