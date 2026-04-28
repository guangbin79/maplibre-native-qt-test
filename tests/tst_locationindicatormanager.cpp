#include <QtTest/QtTest>
#include "locationindicatormanager.h"

Q_DECLARE_METATYPE(LocationIndicatorManager::LocationMode)

class TestLocationIndicatorManager : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testInitialState();
    void testSetLocationFree();
    void testSetLocationIcon();
    void testShowHideLocation();
    void testSetMode();
    void testSetCenterOffset();
    void testMapNotReady();
    void testMapReadyReset();
    void testSetLocationIconBeforeMapReady();
    void testSwitchFreeToFixed();
    void testSwitchFixedToFree();
    void testFixedModeSetLocation();
    void testFreeModeSetLocation();
    void testSetOverlayWidget();
    void testModeSwitchReset();

private:
    LocationIndicatorManager* mgr = nullptr;
};

void TestLocationIndicatorManager::init()
{
    mgr = new LocationIndicatorManager(nullptr);
}

void TestLocationIndicatorManager::cleanup()
{
    delete mgr;
    mgr = nullptr;
}

void TestLocationIndicatorManager::testInitialState()
{
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Free);
    QCOMPARE(mgr->isLocationVisible(), false);
    QCOMPARE(mgr->centerOffset(), 0);
}

void TestLocationIndicatorManager::testSetLocationFree()
{
    mgr->setLocation(39.9, 116.4);
    // No crash with nullptr map
}

void TestLocationIndicatorManager::testSetLocationIcon()
{
    mgr->setLocationIcon(QImage(32, 32, QImage::Format_ARGB32));
    // No crash with nullptr map
}

void TestLocationIndicatorManager::testShowHideLocation()
{
    mgr->showLocation();
    QCOMPARE(mgr->isLocationVisible(), true);

    mgr->hideLocation();
    QCOMPARE(mgr->isLocationVisible(), false);
}

void TestLocationIndicatorManager::testSetMode()
{
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Fixed);

    mgr->setMode(LocationIndicatorManager::LocationMode::Free);
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Free);
}

void TestLocationIndicatorManager::testSetCenterOffset()
{
    mgr->setCenterOffset(200);
    QCOMPARE(mgr->centerOffset(), 200);
}

void TestLocationIndicatorManager::testMapNotReady()
{
    // All operations should not crash with nullptr map and not ready
    mgr->setLocation(39.9, 116.4);
    mgr->setLocationIcon(QImage(32, 32, QImage::Format_ARGB32));
    mgr->showLocation();
    mgr->hideLocation();
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    mgr->setCenterOffset(100);
}

void TestLocationIndicatorManager::testMapReadyReset()
{
    mgr->setMapReady(false);
    mgr->setMapReady(true);
    // No crash
}

void TestLocationIndicatorManager::testSetLocationIconBeforeMapReady()
{
    mgr->setLocationIcon(QImage(32, 32, QImage::Format_ARGB32));
    mgr->showLocation();
    QCOMPARE(mgr->isLocationVisible(), true);
}

void TestLocationIndicatorManager::testSwitchFreeToFixed()
{
    // Default is Free; switch to Fixed with nullptr map — no crash
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Fixed);
}

void TestLocationIndicatorManager::testSwitchFixedToFree()
{
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    mgr->setMode(LocationIndicatorManager::LocationMode::Free);
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Free);
}

void TestLocationIndicatorManager::testFixedModeSetLocation()
{
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    mgr->setMapReady(true);
    mgr->showLocation();
    mgr->setLocation(39.9, 116.4);
    // No crash with nullptr map
}

void TestLocationIndicatorManager::testFreeModeSetLocation()
{
    mgr->setMode(LocationIndicatorManager::LocationMode::Free);
    mgr->setMapReady(true);
    mgr->showLocation();
    mgr->setLocation(39.9, 116.4);
    // No crash with nullptr map
}

void TestLocationIndicatorManager::testSetOverlayWidget()
{
    QWidget w;
    mgr->setOverlayWidget(&w);
    // No crash — overlay stored
}

void TestLocationIndicatorManager::testModeSwitchReset()
{
    // Free → Fixed → Free — verify final mode and no crash
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Fixed);
    mgr->setMode(LocationIndicatorManager::LocationMode::Free);
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Free);
    mgr->setLocation(39.9, 116.4);
    mgr->showLocation();
    mgr->hideLocation();
}

QTEST_MAIN(TestLocationIndicatorManager)
#include "tst_locationindicatormanager.moc"
