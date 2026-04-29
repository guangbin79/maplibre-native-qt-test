#ifndef CAMERAANIMATIONMATH_H
#define CAMERAANIMATIONMATH_H

#include <cmath>
#include <QtGlobal>

namespace CameraMath {

inline double bearingDelta(double from, double to) {
    double delta = std::fmod(to - from, 360.0);
    if (delta > 180.0) delta -= 360.0;
    if (delta < -180.0) delta += 360.0;
    return delta;
}

inline double clampedPitch(double p) {
    return qBound(0.0, p, 60.0);
}

inline double clampedZoom(double z) {
    return qBound(0.0, z, 18.0);
}

inline double easeInOutQuad(double t) {
    return t < 0.5 ? 2.0 * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 2.0) / 2.0;
}

inline double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

} // namespace CameraMath

#endif // CAMERAANIMATIONMATH_H
