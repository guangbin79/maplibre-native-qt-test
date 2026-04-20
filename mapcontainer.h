#pragma once
#include <QWidget>

namespace QMapLibre {
class GLWidget;
class Map;
}

class QPinchGesture;

class MapContainer : public QWidget {
    Q_OBJECT
public:
    explicit MapContainer(QWidget *parent = nullptr);
    QMapLibre::Map *map() const;

signals:
    void zoomChanged(double zoom);
    void bearingChanged(double bearing);
    void tiltChanged(double tilt);
    void centerChanged(double lat, double lon);

protected:
    bool event(QEvent *event) override;

private:
    QMapLibre::GLWidget *m_glWidget;
    double m_lastZoom = 0.0;
    double m_lastBearing = 0.0;
    double m_lastPitch = 0.0;
    double m_lastLat = 0.0;
    double m_lastLon = 0.0;

#ifdef Q_OS_ANDROID
    void handlePinchGesture(QPinchGesture *gesture);

    bool m_gestureActive = false;
    double m_startZoom = 0;
    double m_startBearing = 0;
    double m_startDistance = 0;
    QPointF m_startCenter;
#endif
};
