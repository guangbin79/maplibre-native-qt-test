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
    void testInterpolateAngleWrapForward();
    void testInterpolateAngleWrapBackward();
    void testInterpolateAngleEndpoints();
    void testInterpolateAngle180Ambiguous();
    void testInterpolateAngleAntiJitter();
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

void tst_CameraAnimation::testInterpolateAngleWrapForward() {
    // 350 → 10 should go through 360 (+20°), halfway = 0°
    QCOMPARE(interpolateAngle(350.0, 10.0, 0.5), 0.0);
}

void tst_CameraAnimation::testInterpolateAngleWrapBackward() {
    // 10 → 350 should go backward through 0 (-20°), halfway = 0°
    QCOMPARE(interpolateAngle(10.0, 350.0, 0.5), 0.0);
}

void tst_CameraAnimation::testInterpolateAngleEndpoints() {
    QCOMPARE(interpolateAngle(90.0, 180.0, 0.0), 90.0);
    QCOMPARE(interpolateAngle(90.0, 180.0, 1.0), 180.0);
}

void tst_CameraAnimation::testInterpolateAngle180Ambiguous() {
    // 90 → 270 is a 180° span; either direction is valid
    double result = interpolateAngle(90.0, 270.0, 0.5);
    QVERIFY2(qAbs(result) < 1.0 || qAbs(result - 180.0) < 1.0 || qAbs(result + 180.0) < 1.0,
             "interpolateAngle(90, 270, 0.5) should be ~0 or ~±180");
}

void tst_CameraAnimation::testInterpolateAngleAntiJitter() {
    // Simulate jittery heading stream [0, 5, -3, 8, 2, 0] over 6 updates
    // Each update: from = previous output, to = new input, t = easeInOutQuad(0.33) (~33ms/100ms)
    const double inputs[] = {0.0, 5.0, -3.0, 8.0, 2.0, 0.0};
    const double t = easeInOutQuad(0.33);
    double current = 0.0;
    double maxInputDelta = 0.0;
    double maxOutputDelta = 0.0;
    double prevInput = inputs[0];

    for (int i = 1; i < 6; ++i) {
        double inputDelta = qAbs(inputs[i] - prevInput);
        maxInputDelta = qMax(maxInputDelta, inputDelta);
        prevInput = inputs[i];

        double prev = current;
        current = interpolateAngle(current, inputs[i], t);
        double outputDelta = qAbs(current - prev);
        maxOutputDelta = qMax(maxOutputDelta, outputDelta);
    }
    // Anti-jitter: max output delta must be STRICTLY LESS than max input delta
    QVERIFY2(maxOutputDelta < maxInputDelta,
             "Animation low-pass filter should reduce jitter amplitude");
}

QTEST_MAIN(tst_CameraAnimation)
#include "tst_cameraanimation.moc"
