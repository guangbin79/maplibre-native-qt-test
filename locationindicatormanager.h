#ifndef LOCATIONINDICATORMANAGER_H
#define LOCATIONINDICATORMANAGER_H

#include <QObject>
#include <QImage>
#include <QWidget>

namespace QMapLibre { class Map; }

class LocationIndicatorManager : public QObject {
    Q_OBJECT

public:
    enum class LocationMode { Free, Fixed };

    explicit LocationIndicatorManager(QMapLibre::Map* map, QObject* parent = nullptr);

    void setLocation(double lat, double lon);
    void setLocationIcon(const QImage& icon);
    void setMode(LocationMode mode);
    LocationMode mode() const;
    void showLocation();
    void hideLocation();
    bool isLocationVisible() const;
    void setCenterOffset(int bottomPixels);
    int centerOffset() const;
    void setMapReady(bool ready);
    void setOverlayWidget(QWidget* overlay);
    void repositionOverlay();

private:
    void ensureLayerSetup();
    void rebuildSource();
    QByteArray buildGeoJson() const;
    void applyFixedMode();
    void applyFreeMode();

    QMapLibre::Map* m_map;
    QWidget* m_overlay = nullptr;
    bool m_ready = false;
    bool m_layerSetup = false;
    double m_lat = 0.0;
    double m_lon = 0.0;
    LocationMode m_mode = LocationMode::Free;
    bool m_visible = false;
    QImage m_icon;
    int m_centerOffset = 0;
};

#endif
