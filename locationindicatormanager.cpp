#include "locationindicatormanager.h"

#include <QMapLibre/Map>
#include <QGuiApplication>
#include <QScreen>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMargins>
#include <QLabel>
#include <QPixmap>

LocationIndicatorManager::LocationIndicatorManager(QMapLibre::Map* map, QObject* parent)
    : QObject(parent)
    , m_map(map)
{
}

void LocationIndicatorManager::setMapReady(bool ready)
{
    m_ready = ready;
    if (!ready)
        m_layerSetup = false;
}

void LocationIndicatorManager::setLocation(double lat, double lon)
{
    m_lat = lat;
    m_lon = lon;

    if (!m_ready)
        return;

    if (m_mode == LocationMode::Free && m_visible) {
        ensureLayerSetup();
        rebuildSource();
    }
}

void LocationIndicatorManager::setLocationIcon(const QImage& icon)
{
    m_icon = icon;

    if (!m_layerSetup || !m_map)
        return;

    const double dpr = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->devicePixelRatio() : 1.0;
    QImage scaled = m_icon.scaledToWidth(static_cast<int>(m_icon.width() * dpr), Qt::SmoothTransformation);
    m_map->addImage("location-indicator-icon", scaled);
}

void LocationIndicatorManager::setLocationRotation(double degrees)
{
    m_rotation = degrees;

    if (!m_ready || !m_map)
        return;

    if (m_mode == LocationMode::Free && m_layerSetup) {
        m_map->setLayoutProperty("location-indicator-layer", "icon-rotate", m_rotation);
    } else if (m_mode == LocationMode::Fixed && m_overlay && m_visible) {
        if (m_icon.isNull())
            return;

        auto *label = qobject_cast<QLabel*>(m_overlay);
        if (!label)
            return;

        applyRotatedPixmap(label);
    }
}

double LocationIndicatorManager::locationRotation() const
{
    return m_rotation;
}

void LocationIndicatorManager::setMode(LocationMode mode)
{
    if (mode == m_mode)
        return;
    m_mode = mode;
    if (m_mode == LocationMode::Fixed)
        applyFixedMode();
    else
        applyFreeMode();
}

LocationIndicatorManager::LocationMode LocationIndicatorManager::mode() const
{
    return m_mode;
}

void LocationIndicatorManager::showLocation()
{
    m_visible = true;
    if (m_mode == LocationMode::Fixed) {
        applyFixedMode();
    } else {
        ensureLayerSetup();
        rebuildSource();
        if (m_map)
            m_map->setLayoutProperty("location-indicator-layer", "visibility", "visible");
    }
}

void LocationIndicatorManager::hideLocation()
{
    m_visible = false;
    if (m_overlay)
        m_overlay->hide();
    if (m_map)
        m_map->setLayoutProperty("location-indicator-layer", "visibility", "none");
}

bool LocationIndicatorManager::isLocationVisible() const
{
    return m_visible;
}

void LocationIndicatorManager::setCenterOffset(int bottomPixels)
{
    m_centerOffset = bottomPixels;
    if (m_mode == LocationMode::Fixed && m_map) {
        m_map->setMargins(QMargins(0, 0, 0, m_centerOffset));
        repositionOverlay();
    }
}

int LocationIndicatorManager::centerOffset() const
{
    return m_centerOffset;
}

void LocationIndicatorManager::ensureLayerSetup()
{
    if (m_layerSetup || !m_ready || !m_map)
        return;

    m_map->addSource("location-indicator", QVariantMap{
        {"type", "geojson"},
        {"data", QByteArray("{\"type\":\"FeatureCollection\",\"features\":[]}")}
    });

    m_map->addLayer("location-indicator-layer", QVariantMap{
        {"type", "symbol"},
        {"source", "location-indicator"}
    });

    m_map->setLayoutProperty("location-indicator-layer", "icon-image", "location-indicator-icon");
    m_map->setLayoutProperty("location-indicator-layer", "icon-anchor", "center");
    m_map->setLayoutProperty("location-indicator-layer", "icon-allow-overlap", true);
    m_map->setLayoutProperty("location-indicator-layer", "icon-ignore-placement", true);
    m_map->setLayoutProperty("location-indicator-layer", "icon-rotate", m_rotation);
    m_map->setLayoutProperty("location-indicator-layer", "icon-rotation-alignment", "map");
    m_map->setLayoutProperty("location-indicator-layer", "visibility", "none");

    m_layerSetup = true;

    if (!m_icon.isNull()) {
        const double dpr = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->devicePixelRatio() : 1.0;
        QImage scaled = m_icon.scaledToWidth(static_cast<int>(m_icon.width() * dpr), Qt::SmoothTransformation);
        m_map->addImage("location-indicator-icon", scaled);
    }
}

void LocationIndicatorManager::rebuildSource()
{
    if (!m_layerSetup || !m_ready || !m_map)
        return;

    m_map->updateSource("location-indicator", QVariantMap{{"data", buildGeoJson()}});
}

QByteArray LocationIndicatorManager::buildGeoJson() const
{
    QJsonObject feature;
    feature["type"] = "Feature";

    QJsonObject geometry;
    geometry["type"] = "Point";
    geometry["coordinates"] = QJsonArray{m_lon, m_lat};
    feature["geometry"] = geometry;

    QJsonArray features;
    features.append(feature);

    QJsonObject fc;
    fc["type"] = "FeatureCollection";
    fc["features"] = features;

    return QJsonDocument(fc).toJson(QJsonDocument::Compact);
}

void LocationIndicatorManager::setOverlayWidget(QWidget* overlay)
{
    m_overlay = overlay;
}

void LocationIndicatorManager::applyFixedMode()
{
    if (!m_map) return;
    m_map->setMargins(QMargins(0, 0, 0, m_centerOffset));
    // Fixed 模式下使用 symbol layer 在地图坐标上渲染标注，
    // 通过 margins 偏移地图中心，使标注显示在屏幕特定位置。
    // 这样缩放/旋转/倾斜时标注始终保持在正确的 GPS 坐标上。
    if (m_overlay)
        m_overlay->hide();
    if (m_visible && m_layerSetup) {
        rebuildSource();
        m_map->setLayoutProperty("location-indicator-layer", "visibility", "visible");
    }
    if (m_visible && !m_followingPaused)
        m_map->setCoordinate(QMapLibre::Coordinate(m_lat, m_lon));
}

void LocationIndicatorManager::applyFreeMode()
{
    if (!m_map) return;
    m_map->setMargins(QMargins());
    if (m_overlay)
        m_overlay->hide();
    if (m_visible && m_layerSetup) {
        rebuildSource();
        m_map->setLayoutProperty("location-indicator-layer", "visibility", "visible");
    }
}

void LocationIndicatorManager::applyRotatedPixmap(QLabel *label)
{
    QTransform transform;
    transform.rotate(m_rotation);
    QImage rotated = m_icon.transformed(transform, Qt::SmoothTransformation);
    const double dpr = QGuiApplication::primaryScreen()
                           ? QGuiApplication::primaryScreen()->devicePixelRatio()
                           : 1.0;
    QImage scaled = rotated.scaledToWidth(static_cast<int>(m_icon.width() * dpr), Qt::SmoothTransformation);
    label->setPixmap(QPixmap::fromImage(scaled));
}

void LocationIndicatorManager::repositionOverlay()
{
    if (!m_overlay || !m_overlay->parentWidget()) return;
    QWidget* parent = m_overlay->parentWidget();
    int ow = m_overlay->width();
    int oh = m_overlay->height();
    int pw = parent->width();
    int ph = parent->height();
    int x = (pw - ow) / 2;
    int y = ph - m_centerOffset - oh / 2;
    m_overlay->move(x, y);
}

void LocationIndicatorManager::setFollowingPaused(bool paused)
{
    m_followingPaused = paused;
}

bool LocationIndicatorManager::isFollowingPaused() const
{
    return m_followingPaused;
}

QPair<double, double> LocationIndicatorManager::location() const
{
    return qMakePair(m_lat, m_lon);
}

void LocationIndicatorManager::showLocationOnMap()
{
    if (m_layerSetup && m_map) {
        rebuildSource();
        m_map->setLayoutProperty("location-indicator-layer", "visibility", "visible");
    }
    if (m_overlay)
        m_overlay->hide();
}

void LocationIndicatorManager::restoreFixedDisplay()
{
    if (m_overlay) {
        repositionOverlay();
        m_overlay->show();
    }
    if (m_layerSetup && m_map)
        m_map->setLayoutProperty("location-indicator-layer", "visibility", "none");
}
