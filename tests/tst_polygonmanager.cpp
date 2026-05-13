#include <QtTest/QtTest>
#include "polygonmanager.h"

class TestPolygonManager : public QObject {
    Q_OBJECT

private slots:
    void testDefaultValues();
    void testSetPolygons();
    void testAddPolygon();
    void testAddPolygons();
    void testRemovePolygon();
    void testClearPolygons();
    void testVisibilityShowHide();
    void testSetVisiblePolygonIds();
    void testBoundingBoxForPolygon();
    void testMapNotReady();
    void testAllPolygonIdsDedup();
};

static MapPolygon makePolygon(const QString& id, const QString& polygonId,
                               const QVector<QPair<double, double>>& coords = {{39.9, 116.4}, {31.2, 121.5}, {34.0, 108.9}},
                               const QString& title = "")
{
    MapPolygon poly;
    poly.id = id;
    poly.polygonId = polygonId;
    poly.coordinates = coords;
    poly.fillColor = QColor("#FF5722");
    poly.fillOpacity = 0.5;
    poly.strokeColor = QColor("#000000");
    poly.strokeWidth = 2.0;
    poly.strokeDashed = false;
    poly.title = title;
    return poly;
}

void TestPolygonManager::testDefaultValues()
{
    PolygonManager mgr(nullptr);
    QVERIFY(mgr.allPolygonIds().isEmpty());
    QVERIFY(mgr.visiblePolygonIds().isEmpty());
}

void TestPolygonManager::testSetPolygons()
{
    PolygonManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapPolygon> polys;
    polys.append(makePolygon("p1", "zoneA", {}, "Zone A"));
    polys.append(makePolygon("p2", "zoneB", {}, "Zone B"));

    mgr.setPolygons(polys);

    QStringList ids = mgr.allPolygonIds();
    QCOMPARE(ids.size(), 2);
    QVERIFY(ids.contains("zoneA"));
    QVERIFY(ids.contains("zoneB"));
    QCOMPARE(mgr.visiblePolygonIds().size(), 2);
}

void TestPolygonManager::testAddPolygon()
{
    PolygonManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addPolygon(makePolygon("p1", "zoneA", {}, "Zone A"));

    QStringList ids = mgr.allPolygonIds();
    QCOMPARE(ids.size(), 1);
    QCOMPARE(ids.first(), QString("zoneA"));
    QCOMPARE(mgr.visiblePolygonIds().size(), 1);
}

void TestPolygonManager::testAddPolygons()
{
    PolygonManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapPolygon> polys;
    polys.append(makePolygon("p1", "zoneA", {}, "Zone A"));
    polys.append(makePolygon("p2", "zoneB", {}, "Zone B"));

    mgr.addPolygons(polys);

    QStringList ids = mgr.allPolygonIds();
    QCOMPARE(ids.size(), 2);
    QVERIFY(ids.contains("zoneA"));
    QVERIFY(ids.contains("zoneB"));
    QCOMPARE(mgr.visiblePolygonIds().size(), 2);
}

void TestPolygonManager::testRemovePolygon()
{
    PolygonManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addPolygon(makePolygon("p1", "zoneA", {}, "Zone A"));
    QCOMPARE(mgr.allPolygonIds().size(), 1);

    mgr.removePolygon("p1");
    QVERIFY(mgr.allPolygonIds().isEmpty());
    QVERIFY(mgr.visiblePolygonIds().isEmpty());
}

void TestPolygonManager::testClearPolygons()
{
    PolygonManager mgr(nullptr);
    mgr.setMapReady(true);

    mgr.addPolygon(makePolygon("p1", "zoneA", {}, "Zone A"));
    QCOMPARE(mgr.allPolygonIds().size(), 1);

    mgr.clearPolygons();
    QVERIFY(mgr.allPolygonIds().isEmpty());
    QVERIFY(mgr.visiblePolygonIds().isEmpty());
}

void TestPolygonManager::testVisibilityShowHide()
{
    PolygonManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapPolygon> polys;
    polys.append(makePolygon("p1", "zoneA", {}, "Zone A"));
    polys.append(makePolygon("p2", "zoneB", {}, "Zone B"));

    mgr.setPolygons(polys);
    QCOMPARE(mgr.visiblePolygonIds().size(), 2);

    mgr.hideAllPolygons();
    QVERIFY(mgr.visiblePolygonIds().isEmpty());

    mgr.showAllPolygons();
    QCOMPARE(mgr.visiblePolygonIds().size(), 2);
}

void TestPolygonManager::testSetVisiblePolygonIds()
{
    PolygonManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapPolygon> polys;
    polys.append(makePolygon("p1", "zoneA", {}, "Zone A"));
    polys.append(makePolygon("p2", "zoneB", {}, "Zone B"));

    mgr.setPolygons(polys);
    QCOMPARE(mgr.visiblePolygonIds().size(), 2);

    mgr.setVisiblePolygonIds({"zoneA"});
    QCOMPARE(mgr.visiblePolygonIds().size(), 1);
    QCOMPARE(mgr.visiblePolygonIds().first(), QString("zoneA"));
}

void TestPolygonManager::testBoundingBoxForPolygon()
{
    PolygonManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<QPair<double, double>> coords = {
        {39.9, 116.4},
        {40.0, 116.5},
        {39.8, 116.3}
    };
    mgr.addPolygon(makePolygon("p1", "zoneA", coords, "Zone A"));

    QMapLibre::Coordinate sw, ne;
    bool ok = mgr.boundingBoxForPolygon("zoneA", sw, ne);
    QVERIFY(ok);

    // boundingBoxForPolygon adds padding: 10% of span with 0.005° minimum
    // lat span = 40.0 - 39.8 = 0.2, 10% = 0.02, > 0.005 → pad = 0.02
    // lon span = 116.5 - 116.3 = 0.2, 10% = 0.02, > 0.005 → pad = 0.02
    // sw.lat = 39.8 - 0.02 = 39.78, ne.lat = 40.0 + 0.02 = 40.02
    // sw.lon = 116.3 - 0.02 = 116.28, ne.lon = 116.5 + 0.02 = 116.52
    QVERIFY(ok);

    QCOMPARE(sw.first, 39.78);
    QCOMPARE(ne.first, 40.02);
    QCOMPARE(sw.second, 116.28);
    QCOMPARE(ne.second, 116.52);
}

void TestPolygonManager::testMapNotReady()
{
    PolygonManager mgr(nullptr);

    mgr.addPolygon(makePolygon("p1", "zoneA", {}, "Zone A"));
    QVERIFY(mgr.allPolygonIds().isEmpty());

    mgr.setPolygons({makePolygon("p1", "zoneA", {}, "Zone A")});
    QVERIFY(mgr.allPolygonIds().isEmpty());
}

void TestPolygonManager::testAllPolygonIdsDedup()
{
    PolygonManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapPolygon> polys;
    polys.append(makePolygon("p1", "zoneA", {}, "Zone A"));
    polys.append(makePolygon("p2", "zoneA", {}, "Zone A-2"));
    polys.append(makePolygon("p3", "zoneB", {}, "Zone B"));

    mgr.setPolygons(polys);
    QStringList ids = mgr.allPolygonIds();
    QCOMPARE(ids.size(), 2);
    QVERIFY(ids.contains("zoneA"));
    QVERIFY(ids.contains("zoneB"));
}

QTEST_MAIN(TestPolygonManager)
#include "tst_polygonmanager.moc"
