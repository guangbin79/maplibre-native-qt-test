#include <QtTest/QtTest>
#include "imageryoverlaymanager.h"

class TestImageryOverlay : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testTilesUrl();
    void testConstants();
    void testVisibilityToggles();
    void testNullMapCallOrderSafe();
};

void TestImageryOverlay::testInitialState()
{
    ImageryOverlayManager mgr(nullptr);
    QVERIFY(mgr.isVisible());
}

void TestImageryOverlay::testTilesUrl()
{
    QCOMPARE(ImageryOverlayManager::tilesUrl(),
             QString("http://127.0.0.1:4943/gisserver/mbtiles/{z}/{x}/{y}/imagery"));
}

void TestImageryOverlay::testConstants()
{
    QCOMPARE(QString(ImageryOverlayManager::SOURCE_ID), QString("imagery"));
    QCOMPARE(QString(ImageryOverlayManager::LAYER_ID), QString("satellite"));
}

void TestImageryOverlay::testVisibilityToggles()
{
    ImageryOverlayManager mgr(nullptr);
    mgr.setMapReady(true);
    QVERIFY(mgr.isVisible());
    mgr.setVisible(false);
    QVERIFY(!mgr.isVisible());
    mgr.setVisible(true);
    QVERIFY(mgr.isVisible());
}

void TestImageryOverlay::testNullMapCallOrderSafe()
{
    ImageryOverlayManager mgr(nullptr);
    mgr.setVisible(false);
    QVERIFY(!mgr.isVisible());
    mgr.setMapReady(true);
    QVERIFY(!mgr.isVisible());
    mgr.setVisible(true);
    QVERIFY(mgr.isVisible());
    mgr.setMapReady(false);
    QVERIFY(mgr.isVisible());
}

QTEST_MAIN(TestImageryOverlay)
#include "tst_imageryoverlay.moc"
