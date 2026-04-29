#include <QtTest/QtTest>
#include "cameraanimationmath.h"

using namespace CameraMath;

class tst_CameraAnimation : public QObject {
    Q_OBJECT

private slots:
    void testBearingDelta();
    void testClampedPitch();
    void testClampedZoom();
    void testEaseInOutQuad();
    void testLerp();
};

void tst_CameraAnimation::testBearingDelta() {
    // Forward wrap through 360
    QCOMPARE(bearingDelta(350, 10), 20.0);
    // Backward wrap
    QCOMPARE(bearingDelta(10, 350), -20.0);
    // No change
    QCOMPARE(bearingDelta(180, 180), 0.0);
    // Equivalent bearings
    QCOMPARE(bearingDelta(0, 360), 0.0);
    // 180 degrees — either direction is valid
    double d1 = bearingDelta(0, 180);
    QVERIFY2(qFuzzyCompare(d1, 180.0) || qFuzzyCompare(d1, -180.0),
             "bearingDelta(0, 180) should be +/-180");
    double d2 = bearingDelta(90, 270);
    QVERIFY2(qFuzzyCompare(d2, 180.0) || qFuzzyCompare(d2, -180.0),
             "bearingDelta(90, 270) should be +/-180");
    // Simple forward
    QCOMPARE(bearingDelta(0, 90), 90.0);
    // Simple backward
    QCOMPARE(bearingDelta(90, 0), -90.0);
}

void tst_CameraAnimation::testClampedPitch() {
    QCOMPARE(clampedPitch(-5), 0.0);
    QCOMPARE(clampedPitch(0), 0.0);
    QCOMPARE(clampedPitch(30), 30.0);
    QCOMPARE(clampedPitch(60), 60.0);
    QCOMPARE(clampedPitch(70), 60.0);
}

void tst_CameraAnimation::testClampedZoom() {
    QCOMPARE(clampedZoom(-1), 0.0);
    QCOMPARE(clampedZoom(0), 0.0);
    QCOMPARE(clampedZoom(12), 12.0);
    QCOMPARE(clampedZoom(18), 18.0);
    QCOMPARE(clampedZoom(20), 18.0);
}

void tst_CameraAnimation::testEaseInOutQuad() {
    QCOMPARE(easeInOutQuad(0), 0.0);
    QCOMPARE(easeInOutQuad(1), 1.0);
    QCOMPARE(easeInOutQuad(0.5), 0.5);
    // Quadratic easing: first half = 2t^2
    QVERIFY(qFuzzyCompare(easeInOutQuad(0.25), 0.125));
    // Second half: 1 - (-2t+2)^2/2
    QVERIFY(qFuzzyCompare(easeInOutQuad(0.75), 0.875));
}

void tst_CameraAnimation::testLerp() {
    QCOMPARE(lerp(0, 100, 0.5), 50.0);
    QCOMPARE(lerp(-10, 10, 0.5), 0.0);
    QCOMPARE(lerp(0, 100, 0), 0.0);
    QCOMPARE(lerp(0, 100, 1), 100.0);
    QCOMPARE(lerp(50, 50, 0.5), 50.0);
}

QTEST_MAIN(tst_CameraAnimation)
#include "tst_cameraanimation.moc"
