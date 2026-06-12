#include "locationindicatormanager.h"

#include <QDebug>
#include <QMapLibre/Map>
#include <QMapLibre/Types>
#include <QGuiApplication>
#include <QScreen>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMargins>
#include <QEvent>
#include <QPainter>


LocationIndicatorManager::LocationIndicatorManager(QMapLibre::Map* map, QObject* parent)
    : QObject(parent), m_map(map)
{
    m_parentWidget = qobject_cast<QWidget*>(parent);

    if (m_parentWidget) {
        // Create overlay as child of MapContainer, NOT in any layout.
        // Child widgets not in layout float above layout-managed widgets (GL widget).
        m_overlay = new QLabel(m_parentWidget);
        m_overlay->setAlignment(Qt::AlignCenter);
        m_overlay->setAttribute(Qt::WA_TranslucentBackground);
        m_overlay->hide();

        m_parentWidget->installEventFilter(this);
    }

    if (m_map) {
        connect(m_map, &QMapLibre::Map::mapChanged, this,
                [this](QMapLibre::Map::MapChange change) {
                    if (change == QMapLibre::Map::MapChangeDidFinishLoadingMap) {
                        m_ready = true;
                        if (m_visible) {
                            ensureLayerSetup();
                            rebuildSource();
                        }
                    } else if (change == QMapLibre::Map::MapChangeDidFinishLoadingStyle) {
                        m_layerSetup = false;
                        if (m_ready && m_visible) {
                            ensureLayerSetup();
                            rebuildSource();
                        }
                    } else if (change == QMapLibre::Map::MapChangeRegionWillChange) {
                        if (m_state == State::FixedFollowing) {
                            qDebug() << "[LOC_DBG] WillChange: selfAnim=" << m_selfAnimating;
                        }
                        if (m_state == State::FixedFollowing && !m_selfAnimating) {
                            qDebug() << "[LOC_DBG] *** FALSE DRAG → FixedBrowsing ***";
                            m_state = State::FixedBrowsing;
                            emit followingPausedChanged(true);

                            // Switch from screen-pinned overlay to Symbol Layer at GPS coords
                            if (m_overlay)
                                m_overlay->hide();

                            if (m_layerSetup && m_map) {
                                // Symbol Layer coords already up-to-date from setLocation()
                                m_map->setLayoutProperty("location-indicator-layer",
                                                          "visibility", "visible");
                                m_selfAnimating = true;
                                m_map->setMargins(QMargins(0, 0, 0, 0));
                            }
                        }
                    } else if (change == QMapLibre::Map::MapChangeRegionDidChange
                               || change == QMapLibre::Map::MapChangeRegionDidChangeAnimated) {
                        m_selfAnimating = false;
                        if (m_fixedHeadingMode == FixedHeadingMode::NorthUp
                            && m_state == State::FixedBrowsing
                            && m_layerSetup && m_currentLocation.heading.has_value()) {
                            double bearing = m_map->bearing();
                            double compensated = m_currentLocation.heading.value() - bearing;
                            m_map->setLayoutProperty("location-indicator-layer",
                                                      "icon-rotate", compensated);
                        }
                    }
                });
    }
}

void LocationIndicatorManager::setLocation(const LocationData& data)
{
    bool coordsChanged = (data.latitude != m_currentLocation.latitude
                          || data.longitude != m_currentLocation.longitude);
    bool headingChanged = (data.heading != m_currentLocation.heading);

    m_currentLocation = data;

    if (coordsChanged || headingChanged)
        emit locationChanged(m_currentLocation);

    if (!m_ready)
        return;

    // Always update Symbol Layer coordinates in all visible states.
    // Visibility is controlled by mode (overlay vs Symbol Layer), not by
    // skipping data updates. This ensures instant switching when state changes.
    if (m_state != State::Hidden) {
        ensureLayerSetup();
        rebuildSource();
    }

    if (data.heading.has_value()) {
        m_rotation = data.heading.value();
        if (m_layerSetup && m_map) {
            if (m_mode == LocationMode::Free) {
                m_map->setLayoutProperty("location-indicator-layer",
                                          "icon-rotate", m_rotation);
            } else if (m_mode == LocationMode::Fixed && m_state != State::Hidden) {
                if (m_fixedHeadingMode == FixedHeadingMode::NorthUp) {
                    m_map->setLayoutProperty("location-indicator-layer",
                                              "icon-rotate", m_rotation);
                }
            }
        }

    }
}

const LocationIndicatorManager::LocationData&
LocationIndicatorManager::location() const
{
    return m_currentLocation;
}

void LocationIndicatorManager::setLocationIcon(const QImage& icon)
{
    m_icon = icon;

    if (m_overlay && !icon.isNull()) {
        // Scale overlay icon by DPR to match Symbol Layer rendering size.
        // Symbol Layer registers the icon at (icon.width * dpr), and MapLibre
        // renders it at that physical pixel size on screen.
        const double dpr = QGuiApplication::primaryScreen()
                               ? QGuiApplication::primaryScreen()->devicePixelRatio()
                               : 1.0;
        int scaledW = static_cast<int>(icon.width() * dpr);
        int scaledH = static_cast<int>(icon.height() * dpr);
        QImage scaled = icon.scaled(scaledW, scaledH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        m_overlay->setPixmap(QPixmap::fromImage(scaled));
        m_overlay->setFixedSize(scaledW, scaledH);
    }

    if (!m_layerSetup || !m_map)
        return;

    const double dpr = QGuiApplication::primaryScreen()
                           ? QGuiApplication::primaryScreen()->devicePixelRatio()
                           : 1.0;
    QImage scaled = m_icon.scaledToWidth(
        static_cast<int>(m_icon.width() * dpr), Qt::SmoothTransformation);
    m_map->addImage("location-indicator-icon", scaled);
}

void LocationIndicatorManager::setMode(LocationMode mode)
{
    if (mode == m_mode)
        return;
    m_mode = mode;

    if (m_mode == LocationMode::Fixed) {
        if (m_visible && m_state == State::FreeVisible) {
            m_state = State::FixedFollowing;
        }
        applyFixedMode();
    } else {
        State old = m_state;
        if (m_visible && (old == State::FixedFollowing || old == State::FixedBrowsing)) {
            m_state = State::FreeVisible;
            if (old == State::FixedBrowsing)
                emit followingPausedChanged(false);
        }
        applyFreeMode();
    }
}

LocationIndicatorManager::LocationMode LocationIndicatorManager::mode() const
{
    return m_mode;
}

void LocationIndicatorManager::setFixedHeadingMode(FixedHeadingMode mode)
{
    if (mode == m_fixedHeadingMode)
        return;
    m_fixedHeadingMode = mode;

    if (m_layerSetup && m_map) {
        if (m_fixedHeadingMode == FixedHeadingMode::NorthUp) {
            m_map->setLayoutProperty("location-indicator-layer",
                                      "icon-rotation-alignment", "map");
            m_map->setLayoutProperty("location-indicator-layer",
                                      "icon-rotate", m_rotation);
            if (m_map->bearing() != 0.0) {
                m_selfAnimating = true;
                m_map->setBearing(0.0);
            }
        } else {
            m_map->setLayoutProperty("location-indicator-layer",
                                       "icon-rotation-alignment", "viewport");
            m_map->setLayoutProperty("location-indicator-layer",
                                       "icon-rotate", 0.0);
        }
    }
}

LocationIndicatorManager::FixedHeadingMode
LocationIndicatorManager::fixedHeadingMode() const
{
    return m_fixedHeadingMode;
}

void LocationIndicatorManager::showLocation()
{
    m_visible = true;

    if (m_mode == LocationMode::Free) {
        if (m_state != State::FreeVisible) {
            m_state = State::FreeVisible;
        }
        ensureLayerSetup();
        rebuildSource();
        if (m_map)
            m_map->setLayoutProperty("location-indicator-layer",
                                     "visibility", "visible");
    } else {
        if (m_state != State::FixedFollowing) {
            State old = m_state;
            m_state = State::FixedFollowing;
            if (old == State::FixedBrowsing)
                emit followingPausedChanged(false);
        }

        if (m_overlay) {
            // Set overlay pixmap (DPR-scaled) — no rotation in Fixed mode
            const double dpr = QGuiApplication::primaryScreen()
                                   ? QGuiApplication::primaryScreen()->devicePixelRatio()
                                   : 1.0;
            int scaledW = static_cast<int>(m_icon.width() * dpr);
            int scaledH = static_cast<int>(m_icon.height() * dpr);
            if (!m_icon.isNull()) {
                QImage scaled = m_icon.scaled(scaledW, scaledH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                m_overlay->setPixmap(QPixmap::fromImage(scaled));
                m_overlay->setFixedSize(scaledW, scaledH);
            }
            m_overlay->show();
            m_overlay->raise();
        }

        if (m_layerSetup && m_map) {
            m_map->setLayoutProperty("location-indicator-layer",
                                      "visibility", "none");
        }

        applyFixedMode();
    }
}

void LocationIndicatorManager::hideLocation()
{
    m_visible = false;

    if (m_state != State::Hidden) {
        if (m_state == State::FixedBrowsing)
            emit followingPausedChanged(false);
        m_state = State::Hidden;
    }

    if (m_overlay)
        m_overlay->hide();

    if (m_map)
        m_map->setLayoutProperty("location-indicator-layer",
                                 "visibility", "none");
}

bool LocationIndicatorManager::isLocationVisible() const
{
    return m_visible;
}

void LocationIndicatorManager::setCenterOffset(int bottomPixels)
{
    m_centerOffset = bottomPixels;
    if (m_mode == LocationMode::Fixed && m_map) {
        m_selfAnimating = true;
        m_map->setMargins(QMargins(0, m_centerOffset, 0, 0));
    }
}

int LocationIndicatorManager::centerOffset() const
{
    return m_centerOffset;
}

LocationIndicatorManager::State LocationIndicatorManager::state() const
{
    return m_state;
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

    m_map->setLayoutProperty("location-indicator-layer",
                             "icon-image", "location-indicator-icon");
    m_map->setLayoutProperty("location-indicator-layer",
                             "icon-anchor", "center");
    m_map->setLayoutProperty("location-indicator-layer",
                             "icon-allow-overlap", true);
    m_map->setLayoutProperty("location-indicator-layer",
                             "icon-ignore-placement", true);
    m_map->setLayoutProperty("location-indicator-layer",
                              "icon-rotate",
                              m_mode == LocationMode::Free ? m_rotation :
                              (m_fixedHeadingMode == FixedHeadingMode::NorthUp ? m_rotation : 0.0));
    m_map->setLayoutProperty("location-indicator-layer",
                              "icon-rotation-alignment",
                              m_fixedHeadingMode == FixedHeadingMode::NorthUp ? "map" : "viewport");
    m_map->setLayoutProperty("location-indicator-layer",
                             "visibility", "none");

    m_layerSetup = true;

    if (!m_icon.isNull()) {
        const double dpr = QGuiApplication::primaryScreen()
                               ? QGuiApplication::primaryScreen()->devicePixelRatio()
                               : 1.0;
        QImage scaled = m_icon.scaledToWidth(
            static_cast<int>(m_icon.width() * dpr), Qt::SmoothTransformation);
        m_map->addImage("location-indicator-icon", scaled);
    }
}

void LocationIndicatorManager::rebuildSource()
{
    if (!m_layerSetup || !m_ready || !m_map)
        return;

    m_map->updateSource("location-indicator",
                        QVariantMap{{"data", buildGeoJson()}});
}

void LocationIndicatorManager::updateSourceToCoordinate(double lat, double lon)
{
    if (!m_layerSetup || !m_ready || !m_map)
        return;

    QJsonObject feature;
    feature["type"] = "Feature";

    QJsonObject geometry;
    geometry["type"] = "Point";
    geometry["coordinates"] = QJsonArray{lon, lat};
    feature["geometry"] = geometry;

    QJsonArray features;
    features.append(feature);

    QJsonObject fc;
    fc["type"] = "FeatureCollection";
    fc["features"] = features;

    m_map->updateSource("location-indicator",
                        QVariantMap{{"data", QJsonDocument(fc).toJson(QJsonDocument::Compact)}});
}

QByteArray LocationIndicatorManager::buildGeoJson() const
{
    QJsonObject feature;
    feature["type"] = "Feature";

    QJsonObject geometry;
    geometry["type"] = "Point";
    geometry["coordinates"] = QJsonArray{m_currentLocation.longitude,
                                         m_currentLocation.latitude};
    feature["geometry"] = geometry;

    QJsonArray features;
    features.append(feature);

    QJsonObject fc;
    fc["type"] = "FeatureCollection";
    fc["features"] = features;

    return QJsonDocument(fc).toJson(QJsonDocument::Compact);
}

void LocationIndicatorManager::applyFixedMode()
{
    if (!m_map)
        return;

    qDebug() << "[LOC_DBG] applyFixedMode: state=" << (int)m_state
             << "selfAnim=" << m_selfAnimating
             << "visible=" << m_visible;

    m_selfAnimating = true;
    m_map->setMargins(QMargins(0, m_centerOffset, 0, 0));

    if (m_state == State::FixedFollowing) {
        ensureLayerSetup();
        if (m_layerSetup) {
            rebuildSource();
            m_map->setLayoutProperty("location-indicator-layer",
                                     "visibility", "none");
        }

        m_selfAnimating = true;
        QMapLibre::CameraOptions options;
        options.center = QVariant::fromValue(
            QMapLibre::Coordinate(m_currentLocation.latitude,
                                  m_currentLocation.longitude));
        m_map->jumpTo(options);

        if (m_overlay) {
            repositionOverlay();
        }
    }
}

void LocationIndicatorManager::applyFreeMode()
{
    if (!m_map)
        return;

    if (m_overlay)
        m_overlay->hide();

    m_map->setMargins(QMargins());
    if (m_visible && m_layerSetup) {
        rebuildSource();
        m_map->setLayoutProperty("location-indicator-layer",
                                 "visibility", "visible");
    }
}

bool LocationIndicatorManager::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_parentWidget && event->type() == QEvent::Resize) {
        repositionOverlay();
    }
    return QObject::eventFilter(watched, event);
}

void LocationIndicatorManager::repositionOverlay()
{
    if (!m_overlay || !m_parentWidget)
        return;

    // In Fixed mode, the GPS coordinate is always at the map center.
    // Margins shift the map content area — the visual center of that area
    // is where the overlay should be pinned.
    // QMargins(0, m_centerOffset, 0, 0) means content starts at y=m_centerOffset.
    int pw = m_parentWidget->width();
    int ph = m_parentWidget->height();
    int ow = m_overlay->width();
    int oh = m_overlay->height();

    // Content area center
    int centerX = pw / 2;
    int centerY = m_centerOffset + (ph - m_centerOffset) / 2;

    m_overlay->move(centerX - ow / 2, centerY - oh / 2);
    m_overlay->raise();
}




