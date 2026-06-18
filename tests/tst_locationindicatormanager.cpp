#include <QtTest/QtTest>
#include <QSignalSpy>
#include "locationindicatormanager.h"

Q_DECLARE_METATYPE(LocationIndicatorManager::LocationData)
Q_DECLARE_METATYPE(LocationIndicatorManager::LocationMode)
Q_DECLARE_METATYPE(LocationIndicatorManager::FixedHeadingMode)

// Test subclass that exposes protected members for White-box testing.
class TestableLocationIndicatorManager : public LocationIndicatorManager {
public:
    using LocationIndicatorManager::LocationIndicatorManager;
    using LocationIndicatorManager::computeFollowingFrame;
    using LocationIndicatorManager::computeIconFrame;
    using LocationIndicatorManager::maybeEmitVisualLocation;
    using LocationIndicatorManager::State;

    // Helper setters for protected animation state
    void setState(State s) { m_state = s; }
    void setLayerSetup(bool v) { m_layerSetup = v; }
    void setFollowStart(double lat, double lon, qint64 startTime)
    {
        m_followStartLat = lat; m_followStartLon = lon; m_followStartTime = startTime;
    }
    void setFollowTarget(double lat, double lon)
    {
        m_followTargetLat = lat; m_followTargetLon = lon;
    }
    void setAnimDuration(int ms) { m_animDuration = ms; }
    void setResumeAnimating(bool v) { m_resumeAnimating = v; }
    void setIconStart(double lat, double lon, qint64 startTime)
    {
        m_iconStartLat = lat; m_iconStartLon = lon; m_iconStartTime = startTime;
    }
    void setCurrentLocation(double lat, double lon)
    {
        m_currentLocation.latitude = lat;
        m_currentLocation.longitude = lon;
    }
    void setFollowingPaused(bool p) { m_followingPaused = p; }
};

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

    // Group 6: visualLocationChanged during animation
    void testVisualLocationEmittedDuringFollowingAnimation();
    void testVisualLocationFirstValueNearStart();
    void testVisualLocationLastValueAtTarget();
    void testVisualLocationMonotonicLat();
    void testVisualLocationIconBranchEmits();
    void testVisualLocationSpamSuppression();
    void testVisualLocationInterpolationMidpoint();
    void testVisualLocationNoEmitWhenComplete();

    // Group 6 (continued): edge cases for visualLocationChanged
    void testVisualLocationResumeAnimationEmits();
    void testVisualLocationMidpointAccuracy();
    void testVisualLocationNegativeCoords();
    void testVisualLocationInterruption();
    void testVisualLocationStateIndependentEmit();

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

// ===========================================================================
// Group 6: visualLocationChanged during animation (8 tests)
// ===========================================================================

void TestLocationIndicatorManager::testVisualLocationEmittedDuringFollowingAnimation()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.setState(TestableLocationIndicatorManager::State::FixedFollowing);
    mgr.setFollowingPaused(false);
    mgr.setFollowStart(39.9, 116.4, 0);
    mgr.setFollowTarget(40.0, 116.5);
    mgr.setAnimDuration(1200);

    // Simulate 6 animation ticks
    const qint64 ticks[] = {200, 400, 600, 800, 1000, 1200};
    for (qint64 t : ticks) {
        auto frame = mgr.computeFollowingFrame(t);
        mgr.maybeEmitVisualLocation(frame.lat, frame.lon);
    }

    // Stub is no-op → count = 0, test FAILS (RED)
    QCOMPARE(spy.count(), 6);
}

void TestLocationIndicatorManager::testVisualLocationFirstValueNearStart()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.setState(TestableLocationIndicatorManager::State::FixedFollowing);
    mgr.setFollowingPaused(false);
    mgr.setFollowStart(39.9, 116.4, 0);
    mgr.setFollowTarget(40.0, 116.5);
    mgr.setAnimDuration(1200);

    auto frame = mgr.computeFollowingFrame(33);
    mgr.maybeEmitVisualLocation(frame.lat, frame.lon);

    // Stub is no-op → count = 0, test FAILS (RED)
    QVERIFY(spy.count() == 1);
}

void TestLocationIndicatorManager::testVisualLocationLastValueAtTarget()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.setState(TestableLocationIndicatorManager::State::FixedFollowing);
    mgr.setFollowingPaused(false);
    mgr.setFollowStart(39.9, 116.4, 0);
    mgr.setFollowTarget(40.0, 116.5);
    mgr.setAnimDuration(1200);

    auto frame = mgr.computeFollowingFrame(1200);
    mgr.maybeEmitVisualLocation(frame.lat, frame.lon);

    // Stub is no-op → count = 0, test FAILS (RED)
    QCOMPARE(spy.count(), 1);
}

void TestLocationIndicatorManager::testVisualLocationMonotonicLat()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.setState(TestableLocationIndicatorManager::State::FixedFollowing);
    mgr.setFollowingPaused(false);
    mgr.setFollowStart(39.9, 116.4, 0);
    mgr.setFollowTarget(40.0, 116.5);
    mgr.setAnimDuration(1200);

    for (qint64 t = 0; t <= 1200; t += 100) {
        auto frame = mgr.computeFollowingFrame(t);
        mgr.maybeEmitVisualLocation(frame.lat, frame.lon);
    }

    // Stub is no-op → count = 0, test FAILS (RED)
    QCOMPARE(spy.count(), 13);
}

void TestLocationIndicatorManager::testVisualLocationIconBranchEmits()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.setState(TestableLocationIndicatorManager::State::FixedBrowsing);
    mgr.setLayerSetup(true);
    mgr.setIconStart(39.9, 116.4, 0);
    mgr.setCurrentLocation(40.0, 116.5);
    mgr.setAnimDuration(1200);

    {
        auto frame = mgr.computeIconFrame(600);
        mgr.maybeEmitVisualLocation(frame.lat, frame.lon);
    }
    {
        auto frame = mgr.computeIconFrame(1200);
        mgr.maybeEmitVisualLocation(frame.lat, frame.lon);
    }

    // Stub is no-op → count = 0, test FAILS (RED)
    QCOMPARE(spy.count(), 2);
}

void TestLocationIndicatorManager::testVisualLocationSpamSuppression()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    // Call twice with the same coordinates → first emits, second suppressed
    mgr.maybeEmitVisualLocation(39.9, 116.4);
    mgr.maybeEmitVisualLocation(39.9, 116.4);

    // Real impl: first call emits, second is duplicate-suppressed
    QCOMPARE(spy.count(), 1);
}

void TestLocationIndicatorManager::testVisualLocationInterpolationMidpoint()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.setState(TestableLocationIndicatorManager::State::FixedFollowing);
    mgr.setFollowingPaused(false);
    mgr.setFollowStart(39.9, 116.4, 0);
    mgr.setFollowTarget(40.0, 116.5);
    mgr.setAnimDuration(1200);

    auto frame = mgr.computeFollowingFrame(600);  // progress = 0.5
    QVERIFY(qAbs(frame.lat - 39.95) < 1e-9);
    QVERIFY(qAbs(frame.lon - 116.45) < 1e-9);

    mgr.maybeEmitVisualLocation(frame.lat, frame.lon);

    // Stub is no-op → count = 0, test FAILS (RED)
    QCOMPARE(spy.count(), 1);
}

void TestLocationIndicatorManager::testVisualLocationNoEmitWhenComplete()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.maybeEmitVisualLocation(40.0, 116.5);
    mgr.maybeEmitVisualLocation(40.0, 116.5);

    // Real impl: first call emits, second is duplicate-suppressed
    QCOMPARE(spy.count(), 1);
}

// ===========================================================================
// Group 6 (continued): edge cases for visualLocationChanged (5 tests)
// ============================================================================

void TestLocationIndicatorManager::testVisualLocationResumeAnimationEmits()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.setState(TestableLocationIndicatorManager::State::FixedFollowing);
    mgr.setFollowingPaused(false);
    mgr.setResumeAnimating(true);
    mgr.setFollowStart(39.9, 116.4, 0);
    mgr.setFollowTarget(40.0, 116.5);
    mgr.setAnimDuration(1200);

    const qint64 ticks[] = {100, 200, 300};
    double prevLat = 39.9;
    for (qint64 t : ticks) {
        auto frame = mgr.computeFollowingFrame(t);
        mgr.maybeEmitVisualLocation(frame.lat, frame.lon);
        QVERIFY2(frame.lat >= prevLat - 1e-12, "latitude must not move backward");
        QVERIFY2(frame.lat <= 40.0 + 1e-12, "latitude must not overshoot target");
        prevLat = frame.lat;
    }

    QCOMPARE(spy.count(), 3);

    auto finalFrame = mgr.computeFollowingFrame(300);
    QCOMPARE(finalFrame.complete, true);
    QVERIFY(qAbs(finalFrame.lat - 40.0) < 1e-9);
    QVERIFY(qAbs(finalFrame.lon - 116.5) < 1e-9);
}

void TestLocationIndicatorManager::testVisualLocationMidpointAccuracy()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.setState(TestableLocationIndicatorManager::State::FixedFollowing);
    mgr.setFollowingPaused(false);
    mgr.setFollowStart(0.0, 0.0, 0);
    mgr.setFollowTarget(40.0, 100.0);
    mgr.setAnimDuration(1200);

    auto frame = mgr.computeFollowingFrame(600);

    QVERIFY(qAbs(frame.lat - 20.0) < 1e-9);
    QVERIFY(qAbs(frame.lon - 50.0) < 1e-9);
    QVERIFY(qAbs(frame.progress - 0.5) < 1e-9);
    QVERIFY(!frame.complete);

    mgr.maybeEmitVisualLocation(frame.lat, frame.lon);
    QCOMPARE(spy.count(), 1);
}

void TestLocationIndicatorManager::testVisualLocationNegativeCoords()
{
    TestableLocationIndicatorManager mgr(nullptr);

    mgr.setState(TestableLocationIndicatorManager::State::FixedFollowing);
    mgr.setFollowingPaused(false);
    mgr.setFollowStart(-33.9, -70.6, 0);
    mgr.setFollowTarget(-34.6, -58.4);
    mgr.setAnimDuration(1200);

    auto frame = mgr.computeFollowingFrame(600);

    QVERIFY(qAbs(frame.lat - (-34.25)) < 1e-4);
    QVERIFY(qAbs(frame.lon - (-64.5)) < 1e-4);
    QVERIFY(qAbs(frame.progress - 0.5) < 1e-9);
}

void TestLocationIndicatorManager::testVisualLocationInterruption()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.setState(TestableLocationIndicatorManager::State::FixedFollowing);
    mgr.setFollowingPaused(false);
    mgr.setFollowStart(39.9, 116.4, 0);
    mgr.setFollowTarget(40.0, 116.5);
    mgr.setAnimDuration(1200);

    auto frame1 = mgr.computeFollowingFrame(600);
    QVERIFY(qAbs(frame1.lat - 39.95) < 1e-9);
    QVERIFY(qAbs(frame1.lon - 116.45) < 1e-9);
    mgr.maybeEmitVisualLocation(frame1.lat, frame1.lon);

    mgr.setFollowStart(frame1.lat, frame1.lon, 600);
    mgr.setFollowTarget(40.1, 116.6);

    auto frame2 = mgr.computeFollowingFrame(900);
    const double expectedLat = frame1.lat + (40.1 - frame1.lat) * 0.25;
    const double expectedLon = frame1.lon + (116.6 - frame1.lon) * 0.25;
    QVERIFY(qAbs(frame2.lat - expectedLat) < 1e-9);
    QVERIFY(qAbs(frame2.lon - expectedLon) < 1e-9);
    QVERIFY(qAbs(frame2.lat - 39.9875) < 1e-9);
    QVERIFY(qAbs(frame2.lon - 116.4875) < 1e-9);

    mgr.maybeEmitVisualLocation(frame2.lat, frame2.lon);
    QCOMPARE(spy.count(), 2);
}

void TestLocationIndicatorManager::testVisualLocationStateIndependentEmit()
{
    TestableLocationIndicatorManager mgr(nullptr);
    QSignalSpy spy(&mgr, &LocationIndicatorManager::visualLocationChanged);
    QVERIFY(spy.isValid());

    mgr.maybeEmitVisualLocation(40.0, 116.5);
    QCOMPARE(spy.count(), 1);

    mgr.maybeEmitVisualLocation(40.0, 116.5);
    mgr.maybeEmitVisualLocation(40.0, 116.5);
    QCOMPARE(spy.count(), 1);

    mgr.maybeEmitVisualLocation(40.01, 116.51);
    QCOMPARE(spy.count(), 2);

    mgr.setState(TestableLocationIndicatorManager::State::FreeVisible);
    mgr.maybeEmitVisualLocation(40.01, 116.51);
    QCOMPARE(spy.count(), 2);
    mgr.maybeEmitVisualLocation(40.02, 116.52);
    QCOMPARE(spy.count(), 3);
}

QTEST_MAIN(TestLocationIndicatorManager)
#include "tst_locationindicatormanager.moc"
