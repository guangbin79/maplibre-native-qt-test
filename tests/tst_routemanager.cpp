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

    void testArrowSegmentDoesNotCrash();
    void testArrowSegmentBatchDoesNotCrash();
    void testNoArrowSegmentNoRefCount();
    void testArrowRefCountSameColor();
    void testArrowRefCountDifferentColors();
    void testRemoveArrowSegmentDecrements();
    void testRemoveArrowSegmentsBatchDecrements();
    void testClearSegmentsClearsArrowState();
    void testHideAllRoutesWithArrows();
    void testSetSegmentsReplacesArrowState();
    void testHideAllRoutesPreservesSegments();
    void testShowAllRoutesRestoresAll();
    void testSetSegmentsResetsVisibility();
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

static MapRouteSegment makeArrowSegment(const QString& id, const QString& routeId,
                                         const QColor& color = QColor("#ff0000"),
                                         const QColor& arrowColor = QColor())
{
    MapRouteSegment seg;
    seg.id = id;
    seg.routeId = routeId;
    seg.coordinates = {{39.9, 116.4}, {31.2, 121.5}};
    seg.color = color;
    seg.width = 3.0;
    seg.showArrows = true;
    seg.arrowSize = 1.0;
    seg.arrowColor = arrowColor;
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

void TestRouteManager::testArrowSegmentDoesNotCrash()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    QCOMPARE(mgr.allRouteIds().size(), 1);
    QCOMPARE(mgr.segments().size(), 1);
    QVERIFY(mgr.segments().first().showArrows);
}

void TestRouteManager::testArrowSegmentBatchDoesNotCrash()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapRouteSegment> segs;
    segs.append(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    segs.append(makeArrowSegment("s2", "routeB", QColor("#00ff00")));
    segs.append(makeSegment("s3", "routeC", "No arrows"));

    mgr.addRouteSegments(segs);
    QCOMPARE(mgr.allRouteIds().size(), 3);
    QCOMPARE(mgr.segments().size(), 3);
}

void TestRouteManager::testNoArrowSegmentNoRefCount()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeSegment("s1", "routeA", "Route A"));
    QCOMPARE(mgr.allRouteIds().size(), 1);
    mgr.removeRouteSegment("s1");
    QVERIFY(mgr.allRouteIds().isEmpty());
}

void TestRouteManager::testArrowRefCountSameColor()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    mgr.addRouteSegment(makeArrowSegment("s2", "routeA", QColor("#ff0000")));

    QCOMPARE(mgr.segments().size(), 2);

    mgr.removeRouteSegment("s1");
    QCOMPARE(mgr.segments().size(), 1);
    QCOMPARE(mgr.segments().first().id, QString("s2"));

    mgr.removeRouteSegment("s2");
    QVERIFY(mgr.segments().isEmpty());
}

void TestRouteManager::testArrowRefCountDifferentColors()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    mgr.addRouteSegment(makeArrowSegment("s2", "routeB", QColor("#00ff00")));

    QCOMPARE(mgr.segments().size(), 2);

    mgr.removeRouteSegment("s1");
    QCOMPARE(mgr.segments().size(), 1);
    QCOMPARE(mgr.segments().first().effectiveArrowColor(), QColor("#007f00"));
}

void TestRouteManager::testRemoveArrowSegmentDecrements()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    mgr.removeRouteSegment("s1");
    QVERIFY(mgr.allRouteIds().isEmpty());
}

void TestRouteManager::testRemoveArrowSegmentsBatchDecrements()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapRouteSegment> segs;
    segs.append(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    segs.append(makeArrowSegment("s2", "routeA", QColor("#ff0000")));
    segs.append(makeArrowSegment("s3", "routeB", QColor("#0000ff")));

    mgr.setSegments(segs);
    QCOMPARE(mgr.allRouteIds().size(), 2);

    mgr.removeRouteSegments({"s1", "s2"});
    QCOMPARE(mgr.segments().size(), 1);
    QCOMPARE(mgr.segments().first().id, QString("s3"));
}

void TestRouteManager::testClearSegmentsClearsArrowState()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    mgr.addRouteSegment(makeArrowSegment("s2", "routeB", QColor("#00ff00")));

    QCOMPARE(mgr.segments().size(), 2);

    mgr.clearSegments();
    QVERIFY(mgr.segments().isEmpty());
    QVERIFY(mgr.allRouteIds().isEmpty());

    mgr.addRouteSegment(makeArrowSegment("s3", "routeC", QColor("#ff0000")));
    QCOMPARE(mgr.segments().size(), 1);
}

void TestRouteManager::testHideAllRoutesWithArrows()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    QCOMPARE(mgr.visibleRouteIds().size(), 1);

    mgr.hideAllRoutes();
    QVERIFY(mgr.visibleRouteIds().isEmpty());

    QCOMPARE(mgr.segments().size(), 1);
    QVERIFY(mgr.segments().first().showArrows);

    mgr.showAllRoutes();
    QCOMPARE(mgr.visibleRouteIds().size(), 1);
}

void TestRouteManager::testSetSegmentsReplacesArrowState()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    QCOMPARE(mgr.segments().size(), 1);

    QVector<MapRouteSegment> newSegs;
    newSegs.append(makeArrowSegment("s2", "routeB", QColor("#00ff00")));
    newSegs.append(makeSegment("s3", "routeC", "No arrows"));

    mgr.setSegments(newSegs);
    QCOMPARE(mgr.segments().size(), 2);
    QCOMPARE(mgr.allRouteIds().size(), 2);
    QVERIFY(mgr.segments().at(0).showArrows);
    QVERIFY(!mgr.segments().at(1).showArrows);
}

void TestRouteManager::testHideAllRoutesPreservesSegments()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    // Add segments with arrows from 2 different routes
    mgr.addRouteSegment(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    mgr.addRouteSegment(makeArrowSegment("s2", "routeB", QColor("#00ff00")));

    QCOMPARE(mgr.segments().size(), 2);
    QCOMPARE(mgr.visibleRouteIds().size(), 2);
    QVERIFY(mgr.segments().at(0).showArrows);
    QVERIFY(mgr.segments().at(1).showArrows);

    // Hide all routes
    mgr.hideAllRoutes();

    // Visibility state should be empty
    QVERIFY(mgr.visibleRouteIds().isEmpty());

    // But segments data should still exist (not deleted)
    QCOMPARE(mgr.segments().size(), 2);
    QVERIFY(mgr.segments().at(0).showArrows);
    QVERIFY(mgr.segments().at(1).showArrows);

    // Show all should restore visibility
    mgr.showAllRoutes();
    QCOMPARE(mgr.visibleRouteIds().size(), 2);
}

void TestRouteManager::testShowAllRoutesRestoresAll()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addRouteSegment(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    mgr.addRouteSegment(makeArrowSegment("s2", "routeB", QColor("#00ff00")));
    mgr.addRouteSegment(makeSegment("s3", "routeC", "No arrows"));

    QCOMPARE(mgr.visibleRouteIds().size(), 3);

    // Hide all
    mgr.hideAllRoutes();
    QVERIFY(mgr.visibleRouteIds().isEmpty());

    // Show all should restore ALL 3 route IDs
    mgr.showAllRoutes();
    QCOMPARE(mgr.visibleRouteIds().size(), 3);
    QVERIFY(mgr.visibleRouteIds().contains("routeA"));
    QVERIFY(mgr.visibleRouteIds().contains("routeB"));
    QVERIFY(mgr.visibleRouteIds().contains("routeC"));
}

void TestRouteManager::testSetSegmentsResetsVisibility()
{
    RouteManager mgr(nullptr);
    mgr.setMapReady(true);

    // Add initial segments
    mgr.addRouteSegment(makeArrowSegment("s1", "routeA", QColor("#ff0000")));
    mgr.addRouteSegment(makeArrowSegment("s2", "routeB", QColor("#00ff00")));

    // Hide all
    mgr.hideAllRoutes();
    QVERIFY(mgr.visibleRouteIds().isEmpty());

    // Replace segments via setSegments (should reset visibility to all)
    QVector<MapRouteSegment> newSegs;
    newSegs.append(makeArrowSegment("s3", "routeC", QColor("#0000ff")));
    newSegs.append(makeSegment("s4", "routeD", "Route D"));

    mgr.setSegments(newSegs);
    QCOMPARE(mgr.segments().size(), 2);
    // setSegments should set visibleRouteIds to all (routes "routeC" and "routeD")
    QCOMPARE(mgr.visibleRouteIds().size(), 2);
    QVERIFY(mgr.visibleRouteIds().contains("routeC"));
    QVERIFY(mgr.visibleRouteIds().contains("routeD"));
}

QTEST_MAIN(TestRouteManager)
#include "tst_routemanager.moc"
