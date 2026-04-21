#pragma once
#include <QWidget>
#include <QMapLibre/Types>

namespace QMapLibre {
class GLWidget;
class Map;
}

class MapContainer : public QWidget {
    Q_OBJECT
public:
    struct MapConfig {
        QString styleUrl;
        QMapLibre::Coordinate defaultCoordinate;
        double defaultZoom;
        MapConfig(const QString &url = QString(),
                  const QMapLibre::Coordinate &coord = QMapLibre::Coordinate(36.75, 3.05),
                  double zoom = 8.0)
            : styleUrl(url), defaultCoordinate(coord), defaultZoom(zoom) {}
    };

    explicit MapContainer(const MapConfig &config = MapConfig(), QWidget *parent = nullptr);
    QMapLibre::Map *map() const;

    void setStyle(const QString &styleUrl);
    void setCenter(double lat, double lon);
    void setZoom(double zoom);
    void setBearing(double bearing);
    void setPitch(double pitch);

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
