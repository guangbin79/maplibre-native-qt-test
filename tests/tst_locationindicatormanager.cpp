#include <QtTest/QtTest>
#include <QSignalSpy>
#include "locationindicatormanager.h"

Q_DECLARE_METATYPE(LocationIndicatorManager::LocationData)
Q_DECLARE_METATYPE(LocationIndicatorManager::LocationMode)
Q_DECLARE_METATYPE(LocationIndicatorManager::FixedHeadingMode)

class TestLocationIndicatorManager : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Group 1: Initial state & data storage
    void testInitialState();
    void testDefaultLocation();
    void testSetLocationBasic();
    void testSetLocationWithHeading();
    void testSetLocationWithSpeed();
    void testSetLocationWithAllFields();
    void testSetLocationWithoutHeading();
    void testSetLocationWithoutSpeed();
    void testSetLocationNegativeCoords();

    // Group 2: locationChanged signal
    void testLocationChangedEmitted();
    void testLocationChangedNotEmittedSameCoords();
    void testLocationChangedMultipleCalls();
    void testLocationChangedOnlyHeadingChange();
    void testLocationChangedOnlySpeedChange();
    void testLocationChangedSignalDataIntegrity();

    // Group 3: Visibility
    void testShowHideLocation();
    void testShowLocationIdempotent();
    void testHideLocationIdempotent();
    void testHideWhenAlreadyHiddenNoCrash();

    // Group 4: Mode switching
    void testSetMode();
    void testSetModeIdempotent();
    void testModeSwitchFreeToFixedWhileVisible();
    void testModeSwitchFixedToFreeWhileVisible();
    void testModeSwitchWhenHidden();

    // Group 5: Heading mode, offset, icon, signals
    void testSetFixedHeadingMode();
    void testSetFixedHeadingModeIdempotent();
    void testSetFixedHeadingModeBeforeSetMode();
    void testSetCenterOffset();
    void testSetCenterOffsetMultiple();
    void testSetCenterOffsetNegative();
    void testSetLocationIconNoCrash();
    void testFollowingPausedChangedNotEmittedSpuriously();
    void testFollowingPausedChangedNotEmittedInFreeMode();

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

// ===========================================================================
// Group 1: Initial state & data storage (9 tests)
// ===========================================================================

void TestLocationIndicatorManager::testInitialState()
{
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Free);
    QCOMPARE(mgr->isLocationVisible(), false);
    QCOMPARE(mgr->centerOffset(), 0);
    QCOMPARE(mgr->fixedHeadingMode(), LocationIndicatorManager::FixedHeadingMode::HeadingUp);
}

void TestLocationIndicatorManager::testDefaultLocation()
{
    const auto& loc = mgr->location();
    QCOMPARE(loc.latitude, 0.0);
    QCOMPARE(loc.longitude, 0.0);
    QVERIFY(!loc.heading.has_value());
    QVERIFY(!loc.speed.has_value());
}

void TestLocationIndicatorManager::testSetLocationBasic()
{
    LocationIndicatorManager::LocationData data{39.9, 116.4};
    mgr->setLocation(data);
    const auto& loc = mgr->location();
    QCOMPARE(loc.latitude, 39.9);
    QCOMPARE(loc.longitude, 116.4);
}

void TestLocationIndicatorManager::testSetLocationWithHeading()
{
    LocationIndicatorManager::LocationData data{39.9, 116.4, 90.0};
    mgr->setLocation(data);
    const auto& loc = mgr->location();
    QCOMPARE(loc.latitude, 39.9);
    QCOMPARE(loc.longitude, 116.4);
    QVERIFY(loc.heading.has_value());
    QCOMPARE(loc.heading.value(), 90.0);
}

void TestLocationIndicatorManager::testSetLocationWithSpeed()
{
    LocationIndicatorManager::LocationData data{39.9, 116.4, std::nullopt, 10.0};
    mgr->setLocation(data);
    const auto& loc = mgr->location();
    QCOMPARE(loc.latitude, 39.9);
    QCOMPARE(loc.longitude, 116.4);
    QVERIFY(loc.speed.has_value());
    QCOMPARE(loc.speed.value(), 10.0);
}

void TestLocationIndicatorManager::testSetLocationWithAllFields()
{
    LocationIndicatorManager::LocationData data{39.9, 116.4, 90.0, 10.0};
    mgr->setLocation(data);
    const auto& loc = mgr->location();
    QCOMPARE(loc.latitude, 39.9);
    QCOMPARE(loc.longitude, 116.4);
    QVERIFY(loc.heading.has_value());
    QCOMPARE(loc.heading.value(), 90.0);
    QVERIFY(loc.speed.has_value());
    QCOMPARE(loc.speed.value(), 10.0);
}

void TestLocationIndicatorManager::testSetLocationWithoutHeading()
{
    LocationIndicatorManager::LocationData data{39.9, 116.4};
    mgr->setLocation(data);
    const auto& loc = mgr->location();
    QVERIFY(!loc.heading.has_value());
}

void TestLocationIndicatorManager::testSetLocationWithoutSpeed()
{
    LocationIndicatorManager::LocationData data{39.9, 116.4, 90.0};
    mgr->setLocation(data);
    const auto& loc = mgr->location();
    QVERIFY(loc.heading.has_value());
    QVERIFY(!loc.speed.has_value());
}

void TestLocationIndicatorManager::testSetLocationNegativeCoords()
{
    LocationIndicatorManager::LocationData data{-33.8688, 151.2093};
    mgr->setLocation(data);
    const auto& loc = mgr->location();
    QCOMPARE(loc.latitude, -33.8688);
    QCOMPARE(loc.longitude, 151.2093);
}

// ===========================================================================
// Group 2: locationChanged signal (6 tests)
// ===========================================================================

void TestLocationIndicatorManager::testLocationChangedEmitted()
{
    QSignalSpy spy(mgr, &LocationIndicatorManager::locationChanged);
    QVERIFY(spy.isValid());
    mgr->setLocation({39.9, 116.4});
    QCOMPARE(spy.count(), 1);
    auto loc = spy.at(0).at(0).value<LocationIndicatorManager::LocationData>();
    QCOMPARE(loc.latitude, 39.9);
    QCOMPARE(loc.longitude, 116.4);
}

void TestLocationIndicatorManager::testLocationChangedNotEmittedSameCoords()
{
    mgr->setLocation({39.9, 116.4, 90.0});
    QSignalSpy spy(mgr, &LocationIndicatorManager::locationChanged);
    QVERIFY(spy.isValid());
    mgr->setLocation({39.9, 116.4, 90.0, 10.0});
    QCOMPARE(spy.count(), 0);
}

void TestLocationIndicatorManager::testLocationChangedMultipleCalls()
{
    QSignalSpy spy(mgr, &LocationIndicatorManager::locationChanged);
    QVERIFY(spy.isValid());
    mgr->setLocation({39.9, 116.4});
    mgr->setLocation({40.0, 116.4});
    mgr->setLocation({40.0, 116.5});
    QCOMPARE(spy.count(), 3);
}

void TestLocationIndicatorManager::testLocationChangedOnlyHeadingChange()
{
    mgr->setLocation({39.9, 116.4, 90.0});
    QSignalSpy spy(mgr, &LocationIndicatorManager::locationChanged);
    QVERIFY(spy.isValid());
    mgr->setLocation({39.9, 116.4, 180.0});
    QCOMPARE(spy.count(), 1);
}

void TestLocationIndicatorManager::testLocationChangedOnlySpeedChange()
{
    mgr->setLocation({39.9, 116.4, std::nullopt, 10.0});
    QSignalSpy spy(mgr, &LocationIndicatorManager::locationChanged);
    QVERIFY(spy.isValid());
    mgr->setLocation({39.9, 116.4, std::nullopt, 20.0});
    QCOMPARE(spy.count(), 0);
}

void TestLocationIndicatorManager::testLocationChangedSignalDataIntegrity()
{
    QSignalSpy spy(mgr, &LocationIndicatorManager::locationChanged);
    QVERIFY(spy.isValid());
    mgr->setLocation({39.9, 116.4, 90.0, 10.0});
    QCOMPARE(spy.count(), 1);
    auto loc = spy.at(0).at(0).value<LocationIndicatorManager::LocationData>();
    QCOMPARE(loc.latitude, 39.9);
    QCOMPARE(loc.longitude, 116.4);
    QVERIFY(loc.heading.has_value());
    QCOMPARE(loc.heading.value(), 90.0);
    QVERIFY(loc.speed.has_value());
    QCOMPARE(loc.speed.value(), 10.0);
}

// ===========================================================================
// Group 3: Visibility (4 tests)
// ===========================================================================

void TestLocationIndicatorManager::testShowHideLocation()
{
    mgr->showLocation();
    QVERIFY(mgr->isLocationVisible());
    mgr->hideLocation();
    QVERIFY(!mgr->isLocationVisible());
}

void TestLocationIndicatorManager::testShowLocationIdempotent()
{
    mgr->showLocation();
    QVERIFY(mgr->isLocationVisible());
    mgr->showLocation();
    QVERIFY(mgr->isLocationVisible());
}

void TestLocationIndicatorManager::testHideLocationIdempotent()
{
    mgr->hideLocation();
    QVERIFY(!mgr->isLocationVisible());
    mgr->hideLocation();
    QVERIFY(!mgr->isLocationVisible());
}

void TestLocationIndicatorManager::testHideWhenAlreadyHiddenNoCrash()
{
    QVERIFY(!mgr->isLocationVisible());
    mgr->hideLocation();
    QVERIFY(!mgr->isLocationVisible());
}

// ===========================================================================
// Group 4: Mode switching (5 tests)
// ===========================================================================

void TestLocationIndicatorManager::testSetMode()
{
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Fixed);
    mgr->setMode(LocationIndicatorManager::LocationMode::Free);
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Free);
}

void TestLocationIndicatorManager::testSetModeIdempotent()
{
    mgr->setMode(LocationIndicatorManager::LocationMode::Free);
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Free);
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Fixed);
}

void TestLocationIndicatorManager::testModeSwitchFreeToFixedWhileVisible()
{
    mgr->showLocation();
    QVERIFY(mgr->isLocationVisible());
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Free);
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    QVERIFY(mgr->isLocationVisible());
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Fixed);
}

void TestLocationIndicatorManager::testModeSwitchFixedToFreeWhileVisible()
{
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    mgr->showLocation();
    QVERIFY(mgr->isLocationVisible());
    mgr->setMode(LocationIndicatorManager::LocationMode::Free);
    QVERIFY(mgr->isLocationVisible());
    QCOMPARE(mgr->mode(), LocationIndicatorManager::LocationMode::Free);
}

void TestLocationIndicatorManager::testModeSwitchWhenHidden()
{
    QVERIFY(!mgr->isLocationVisible());
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    QVERIFY(!mgr->isLocationVisible());
    mgr->setMode(LocationIndicatorManager::LocationMode::Free);
    QVERIFY(!mgr->isLocationVisible());
}

// ===========================================================================
// Group 5: Heading mode, offset, icon, signals (9 tests)
// ===========================================================================

void TestLocationIndicatorManager::testSetFixedHeadingMode()
{
    mgr->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::NorthUp);
    QCOMPARE(mgr->fixedHeadingMode(), LocationIndicatorManager::FixedHeadingMode::NorthUp);
    mgr->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::HeadingUp);
    QCOMPARE(mgr->fixedHeadingMode(), LocationIndicatorManager::FixedHeadingMode::HeadingUp);
}

void TestLocationIndicatorManager::testSetFixedHeadingModeIdempotent()
{
    mgr->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::HeadingUp);
    QCOMPARE(mgr->fixedHeadingMode(), LocationIndicatorManager::FixedHeadingMode::HeadingUp);
    mgr->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::NorthUp);
    mgr->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::NorthUp);
    QCOMPARE(mgr->fixedHeadingMode(), LocationIndicatorManager::FixedHeadingMode::NorthUp);
}

void TestLocationIndicatorManager::testSetFixedHeadingModeBeforeSetMode()
{
    mgr->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::NorthUp);
    QCOMPARE(mgr->fixedHeadingMode(), LocationIndicatorManager::FixedHeadingMode::NorthUp);
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    QCOMPARE(mgr->fixedHeadingMode(), LocationIndicatorManager::FixedHeadingMode::NorthUp);
}

void TestLocationIndicatorManager::testSetCenterOffset()
{
    mgr->setCenterOffset(200);
    QCOMPARE(mgr->centerOffset(), 200);
}

void TestLocationIndicatorManager::testSetCenterOffsetMultiple()
{
    mgr->setCenterOffset(100);
    QCOMPARE(mgr->centerOffset(), 100);
    mgr->setCenterOffset(300);
    QCOMPARE(mgr->centerOffset(), 300);
    mgr->setCenterOffset(0);
    QCOMPARE(mgr->centerOffset(), 0);
}

void TestLocationIndicatorManager::testSetCenterOffsetNegative()
{
    mgr->setCenterOffset(-50);
    QCOMPARE(mgr->centerOffset(), -50);
}

void TestLocationIndicatorManager::testSetLocationIconNoCrash()
{
    mgr->setLocationIcon(QImage(32, 32, QImage::Format_ARGB32));
}

void TestLocationIndicatorManager::testFollowingPausedChangedNotEmittedSpuriously()
{
    QSignalSpy spy(mgr, &LocationIndicatorManager::followingPausedChanged);
    QVERIFY(spy.isValid());
    mgr->setLocation({39.9, 116.4});
    mgr->showLocation();
    mgr->hideLocation();
    mgr->setMode(LocationIndicatorManager::LocationMode::Fixed);
    mgr->setMode(LocationIndicatorManager::LocationMode::Free);
    QCOMPARE(spy.count(), 0);
}

void TestLocationIndicatorManager::testFollowingPausedChangedNotEmittedInFreeMode()
{
    QSignalSpy spy(mgr, &LocationIndicatorManager::followingPausedChanged);
    QVERIFY(spy.isValid());
    mgr->showLocation();
    mgr->setLocation({39.9, 116.4});
    mgr->setLocation({40.0, 116.5});
    mgr->hideLocation();
    QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(TestLocationIndicatorManager)
#include "tst_locationindicatormanager.moc"
