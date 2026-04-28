#include <QtTest/QtTest>
#include "annotationmanager.h"

class TestAnnotationManager : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testSetAnnotations();
    void testAddAnnotation();
    void testRemoveAnnotation();
    void testVisibleIds();
    void testShowAllAnnotations();
    void testHideAllAnnotations();
    void testIconRefCounting();
    void testRemoveLastIconRef();
    void testClearAnnotations();
    void testAddAnnotationsBatch();
    void testRemoveAnnotationsBatch();
    void testSetVisibleIdsSubset();
    void testMapNotReady();
};

void TestAnnotationManager::testInitialState()
{
    AnnotationManager mgr(nullptr);
    QVERIFY(mgr.allIds().isEmpty());
    QVERIFY(mgr.visibleIds().isEmpty());
}

void TestAnnotationManager::testSetAnnotations()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapAnnotation> anns;
    MapAnnotation a1;
    a1.id = "a1";
    a1.latitude = 39.9;
    a1.longitude = 116.4;
    a1.title = "Beijing";
    a1.iconName = "marker";
    anns.append(a1);

    MapAnnotation a2;
    a2.id = "a2";
    a2.latitude = 31.2;
    a2.longitude = 121.5;
    a2.title = "Shanghai";
    a2.iconName = "pin";
    anns.append(a2);

    mgr.setAnnotations(anns, {});

    QStringList ids = mgr.allIds();
    QCOMPARE(ids.size(), 2);
    QVERIFY(ids.contains("a1"));
    QVERIFY(ids.contains("a2"));
    QCOMPARE(mgr.visibleIds().size(), 2);
}

void TestAnnotationManager::testAddAnnotation()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    MapAnnotation a;
    a.id = "a1";
    a.latitude = 39.9;
    a.longitude = 116.4;
    a.title = "Beijing";
    a.iconName = "marker";

    mgr.addAnnotation(a);

    QStringList ids = mgr.allIds();
    QCOMPARE(ids.size(), 1);
    QCOMPARE(ids.first(), QString("a1"));
    QCOMPARE(mgr.visibleIds().size(), 1);
}

void TestAnnotationManager::testRemoveAnnotation()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    MapAnnotation a;
    a.id = "a1";
    a.latitude = 39.9;
    a.longitude = 116.4;
    a.title = "Beijing";
    a.iconName = "marker";

    mgr.addAnnotation(a);
    QCOMPARE(mgr.allIds().size(), 1);

    mgr.removeAnnotation("a1");
    QVERIFY(mgr.allIds().isEmpty());
    QVERIFY(mgr.visibleIds().isEmpty());
}

void TestAnnotationManager::testVisibleIds()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapAnnotation> anns;
    MapAnnotation a1;
    a1.id = "a1";
    a1.latitude = 39.9;
    a1.longitude = 116.4;
    a1.title = "Beijing";
    a1.iconName = "marker";
    anns.append(a1);

    MapAnnotation a2;
    a2.id = "a2";
    a2.latitude = 31.2;
    a2.longitude = 121.5;
    a2.title = "Shanghai";
    a2.iconName = "pin";
    anns.append(a2);

    mgr.setAnnotations(anns, {});
    QCOMPARE(mgr.visibleIds().size(), 2);

    mgr.setVisibleIds({"a1"});
    QCOMPARE(mgr.visibleIds().size(), 1);
    QCOMPARE(mgr.visibleIds().first(), QString("a1"));
}

void TestAnnotationManager::testShowAllAnnotations()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapAnnotation> anns;
    MapAnnotation a1;
    a1.id = "a1";
    a1.latitude = 39.9;
    a1.longitude = 116.4;
    a1.title = "Beijing";
    a1.iconName = "marker";
    anns.append(a1);

    MapAnnotation a2;
    a2.id = "a2";
    a2.latitude = 31.2;
    a2.longitude = 121.5;
    a2.title = "Shanghai";
    a2.iconName = "pin";
    anns.append(a2);

    mgr.setAnnotations(anns, {});
    mgr.hideAllAnnotations();
    QVERIFY(mgr.visibleIds().isEmpty());

    mgr.showAllAnnotations();
    QCOMPARE(mgr.visibleIds().size(), 2);
}

void TestAnnotationManager::testHideAllAnnotations()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    MapAnnotation a;
    a.id = "a1";
    a.latitude = 39.9;
    a.longitude = 116.4;
    a.title = "Beijing";
    a.iconName = "marker";

    mgr.addAnnotation(a);
    QCOMPARE(mgr.visibleIds().size(), 1);

    mgr.hideAllAnnotations();
    QVERIFY(mgr.visibleIds().isEmpty());
}

void TestAnnotationManager::testIconRefCounting()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    QImage icon(16, 16, QImage::Format_ARGB32);
    icon.fill(Qt::red);

    MapAnnotation a1;
    a1.id = "a1";
    a1.latitude = 39.9;
    a1.longitude = 116.4;
    a1.title = "Beijing";
    a1.iconName = "marker";

    MapAnnotation a2;
    a2.id = "a2";
    a2.latitude = 31.2;
    a2.longitude = 121.5;
    a2.title = "Shanghai";
    a2.iconName = "marker";

    mgr.addAnnotation(a1, icon);
    mgr.addAnnotation(a2, icon);

    QStringList ids = mgr.allIds();
    QCOMPARE(ids.size(), 2);

    mgr.removeAnnotation("a1");
    ids = mgr.allIds();
    QCOMPARE(ids.size(), 1);
    QCOMPARE(ids.first(), QString("a2"));
}

void TestAnnotationManager::testRemoveLastIconRef()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    QImage icon(16, 16, QImage::Format_ARGB32);
    icon.fill(Qt::red);

    MapAnnotation a;
    a.id = "a1";
    a.latitude = 39.9;
    a.longitude = 116.4;
    a.title = "Beijing";
    a.iconName = "marker";

    mgr.addAnnotation(a, icon);
    QCOMPARE(mgr.allIds().size(), 1);

    mgr.removeAnnotation("a1");
    QVERIFY(mgr.allIds().isEmpty());
}

void TestAnnotationManager::testClearAnnotations()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    MapAnnotation a;
    a.id = "a1";
    a.latitude = 39.9;
    a.longitude = 116.4;
    a.title = "Beijing";
    a.iconName = "marker";

    mgr.addAnnotation(a);
    QCOMPARE(mgr.allIds().size(), 1);

    mgr.clearAnnotations();
    QVERIFY(mgr.allIds().isEmpty());
    QVERIFY(mgr.visibleIds().isEmpty());
}

void TestAnnotationManager::testAddAnnotationsBatch()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapAnnotation> anns;
    MapAnnotation a1;
    a1.id = "a1";
    a1.latitude = 39.9;
    a1.longitude = 116.4;
    a1.title = "Beijing";
    a1.iconName = "marker";
    anns.append(a1);

    MapAnnotation a2;
    a2.id = "a2";
    a2.latitude = 31.2;
    a2.longitude = 121.5;
    a2.title = "Shanghai";
    a2.iconName = "pin";
    anns.append(a2);

    mgr.addAnnotations(anns, {});

    QStringList ids = mgr.allIds();
    QCOMPARE(ids.size(), 2);
    QVERIFY(ids.contains("a1"));
    QVERIFY(ids.contains("a2"));
}

void TestAnnotationManager::testRemoveAnnotationsBatch()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapAnnotation> anns;
    MapAnnotation a1;
    a1.id = "a1";
    a1.latitude = 39.9;
    a1.longitude = 116.4;
    a1.title = "Beijing";
    a1.iconName = "marker";
    anns.append(a1);

    MapAnnotation a2;
    a2.id = "a2";
    a2.latitude = 31.2;
    a2.longitude = 121.5;
    a2.title = "Shanghai";
    a2.iconName = "pin";
    anns.append(a2);

    MapAnnotation a3;
    a3.id = "a3";
    a3.latitude = 23.1;
    a3.longitude = 113.3;
    a3.title = "Guangzhou";
    a3.iconName = "flag";
    anns.append(a3);

    mgr.setAnnotations(anns, {});
    QCOMPARE(mgr.allIds().size(), 3);

    mgr.removeAnnotations({"a1", "a2"});
    QStringList ids = mgr.allIds();
    QCOMPARE(ids.size(), 1);
    QCOMPARE(ids.first(), QString("a3"));
}

void TestAnnotationManager::testSetVisibleIdsSubset()
{
    AnnotationManager mgr(nullptr);
    mgr.setMapReady(true);

    QVector<MapAnnotation> anns;
    for (int i = 0; i < 5; ++i) {
        MapAnnotation a;
        a.id = QString("a%1").arg(i);
        a.latitude = 30.0 + i;
        a.longitude = 110.0 + i;
        a.title = QString("City %1").arg(i);
        a.iconName = "marker";
        anns.append(a);
    }

    mgr.setAnnotations(anns, {});
    QCOMPARE(mgr.visibleIds().size(), 5);

    mgr.setVisibleIds({"a0", "a2", "a4"});
    QStringList visible = mgr.visibleIds();
    QCOMPARE(visible.size(), 3);
    QVERIFY(visible.contains("a0"));
    QVERIFY(visible.contains("a2"));
    QVERIFY(visible.contains("a4"));
}

void TestAnnotationManager::testMapNotReady()
{
    AnnotationManager mgr(nullptr);

    MapAnnotation a;
    a.id = "a1";
    a.latitude = 39.9;
    a.longitude = 116.4;
    a.title = "Beijing";
    a.iconName = "marker";

    mgr.addAnnotation(a);
    QVERIFY(mgr.allIds().isEmpty());

    mgr.setAnnotations({a}, {});
    QVERIFY(mgr.allIds().isEmpty());
}

QTEST_MAIN(TestAnnotationManager)
#include "tst_annotationmanager.moc"
