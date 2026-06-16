#include "locationindicatormanager.h"
#include "mapcontainer.h"

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
#include <QtMath>
#include <QDateTime>


LocationIndicatorManager::LocationIndicatorManager(MapContainer* container)
    : QObject(container), m_map(nullptr), m_mapContainer(container)
{
    if (container) {
        m_parentWidget = container;
    }

    if (m_parentWidget) {
        m_parentWidget->installEventFilter(this);
    }
    // Unified animation timer (~30fps)
    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(33);
    connect(m_animTimer, &QTimer::timeout, this, &LocationIndicatorManager::onAnimStep);

    // Resume timer (single-shot) — restores Fixed mode after user stops dragging
    m_resumeTimer = new QTimer(this);
    m_resumeTimer->setSingleShot(true);
    connect(m_resumeTimer, &QTimer::timeout, this, [this]() {
        m_followingPaused = false;
        if (m_targetZoom >= 0) {
            m_selfAnimating = true;
            m_map->setZoom(m_targetZoom);
        }
        if (m_targetPitch >= 0) {
            m_selfAnimating = true;
            m_map->setPitch(m_targetPitch);
        }
        showLocation();
        setLocation(m_currentLocation);
    });

}

void LocationIndicatorManager::initMap(QMapLibre::Map* map)
{
    m_map = map;

    // If the map was already loaded before initMap() was called (mapReady signal
    // fires AFTER DidFinishLoadingMap), set m_ready directly.
    if (m_map && m_mapContainer && m_mapContainer->isMapReady()) {
        m_ready = true;
    }

    // Create overlay lazily — must happen AFTER the GL widget is realized
    // (i.e., after the MapContainer has been shown), otherwise QOpenGLWidget
    // renders on top of the QLabel. initMap() is called from mapReady signal,
    // which fires after the map is fully loaded and the GL widget is rendering.
    if (m_mapContainer && !m_overlay) {
        m_overlay = new QLabel(m_mapContainer);
        m_overlay->setAlignment(Qt::AlignCenter);
        m_overlay->setAttribute(Qt::WA_TranslucentBackground);
        m_overlay->hide();

        // Apply stored icon if setLocationIcon() was called before initMap()
        if (!m_icon.isNull()) {
            const double dpr = QGuiApplication::primaryScreen()
                ? QGuiApplication::primaryScreen()->devicePixelRatio()
                : 1.0;
            int scaledW = static_cast<int>(m_icon.width() * dpr);
            int scaledH = static_cast<int>(m_icon.height() * dpr);
            QImage scaled = m_icon.scaled(scaledW, scaledH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            m_overlay->setPixmap(QPixmap::fromImage(scaled));
            m_overlay->setFixedSize(scaledW, scaledH);
        }
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
                        if (m_state == State::FixedFollowing && !m_selfAnimating) {
                            m_state = State::FixedBrowsing;

                            m_followingPaused = true;
                            if (m_animTimer)
                                m_animTimer->stop();
                            if (m_resumeTimer) {
                                m_resumeTimer->setInterval(m_fixedTouchResumeTimeout);
                                m_resumeTimer->start();
                            }

                            if (m_overlay)
                                m_overlay->hide();

                            if (m_layerSetup && m_map) {
                                // Start icon at current map center (where overlay was),
                                // then animate to GPS position — smooth transition
                                auto coord = m_map->coordinate();
                                m_displayLat = coord.first;
                                m_displayLon = coord.second;
                                m_iconStartLat = coord.first;
                                m_iconStartLon = coord.second;
                                m_iconStartTime = QDateTime::currentMSecsSinceEpoch();
                                updateSourceToCoordinate(m_displayLat, m_displayLon);
                                m_map->setLayoutProperty("location-indicator-layer",
                                                           "visibility", "visible");
                                if (m_animTimer && !m_animTimer->isActive())
                                    m_animTimer->start();
                            }
                        }
                    } else if (change == QMapLibre::Map::MapChangeRegionDidChange
                               || change == QMapLibre::Map::MapChangeRegionDidChangeAnimated) {
                        // Defer reset to next event loop iteration — allows multiple
                        // consecutive self-initiated map operations (setMargins, jumpTo,
                        // setZoom, setPitch in applyFixedMode) to all be protected.
                        // If reset synchronously, the 2nd/3rd/4th operation would be
                        // mistaken for a user drag, hiding the overlay.
                        QMetaObject::invokeMethod(this, [this]() {
                            m_selfAnimating = false;
                        }, Qt::QueuedConnection);
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

    if (m_displayLat == 0.0 && m_displayLon == 0.0) {
        m_displayLat = data.latitude;
        m_displayLon = data.longitude;
    }

    if (coordsChanged || headingChanged)
        emit locationChanged(m_currentLocation);

    if (!m_ready)
        return;

    if (m_state != State::Hidden) {
        ensureLayerSetup();

        if (m_state == State::FreeVisible || m_state == State::FixedBrowsing) {
            if (coordsChanged && m_animTimer && !m_animTimer->isActive())
                m_animTimer->start();
            else if (!m_animTimer || !m_animTimer->isActive())
                updateSourceToCoordinate(m_displayLat, m_displayLon);
        } else {
            rebuildSource();
        }
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

        if (m_state == State::FixedFollowing && m_overlay && headingChanged
            && m_fixedHeadingMode == FixedHeadingMode::NorthUp) {
            updateOverlayRotation();
        }
    }

    // Store follow targets for onAnimStep
    m_followTargetLat = data.latitude;
    m_followTargetLon = data.longitude;
    if (data.heading.has_value()) {
        m_targetBearing = data.heading.value();
    }

    // Record animation start positions for time-deadline interpolation
    if (m_map && m_state == State::FixedFollowing && !m_followingPaused) {
        auto coord = m_map->coordinate();
        m_followStartLat = coord.first;
        m_followStartLon = coord.second;
        m_followStartBearing = m_map->bearing();
        m_followStartTime = QDateTime::currentMSecsSinceEpoch();
    }
    if (m_state == State::FreeVisible || m_state == State::FixedBrowsing) {
        m_iconStartLat = m_displayLat;
        m_iconStartLon = m_displayLon;
        m_iconStartTime = QDateTime::currentMSecsSinceEpoch();
    }

    // Start follow timer in Fixed+FixedFollowing if not paused
    if (m_mode == LocationMode::Fixed && m_state == State::FixedFollowing
        && !m_followingPaused && m_animTimer) {
        if (!m_animTimer->isActive())
            m_animTimer->start();
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

    if (m_overlay && m_visible && m_mode == LocationMode::Fixed && m_state == State::FixedFollowing) {
        updateOverlayRotation();
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
        m_displayLat = m_currentLocation.latitude;
        m_displayLon = m_currentLocation.longitude;
        updateSourceToCoordinate(m_displayLat, m_displayLon);
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
        m_followingPaused = false;

        m_animTimer->stop();

        if (m_overlay) {
            updateOverlayRotation();
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

int LocationIndicatorManager::effectiveCenterOffset() const
{
    if (m_mode == LocationMode::Fixed && m_fixedHeadingMode == FixedHeadingMode::NorthUp)
        return 0;
    return m_centerOffset;
}

void LocationIndicatorManager::setZoom(double zoom)
{
    m_targetZoom = zoom;
    if (m_state == State::FixedFollowing && m_map) {
        m_map->setZoom(zoom);
    }
}

void LocationIndicatorManager::setPitch(double pitch)
{
    m_targetPitch = pitch;
    if (m_state == State::FixedFollowing && m_map) {
        m_map->setPitch(pitch);
    }
}

void LocationIndicatorManager::setAnimDuration(int ms)
{
    m_animDuration = qMax(100, ms);
}

int LocationIndicatorManager::animDuration() const
{
    return m_animDuration;
}

void LocationIndicatorManager::setFixedTouchResumeTimeout(int ms)
{
    m_fixedTouchResumeTimeout = ms;
}

int LocationIndicatorManager::fixedTouchResumeTimeout() const
{
    return m_fixedTouchResumeTimeout;
}

void LocationIndicatorManager::pauseFollowing()
{
    // Don't stop m_animTimer — it's needed for icon animation in FixedBrowsing state.
    // The state machine handles everything:
    // - FixedFollowing + m_followingPaused=true → onAnimStep skips map following
    // - FixedBrowsing → onAnimStep runs icon animation
    // Stopping the timer here would freeze the icon during drag, because
    // pauseFollowing() is called on every MouseMove event.

    // Restart resume timer on every user interaction (called on every MouseMove).
    // This keeps the timeout alive as long as the user is actively dragging.
    // The timer only fires after the user stops dragging for m_fixedTouchResumeTimeout ms.
    if (m_resumeTimer && m_state == State::FixedBrowsing) {
        m_resumeTimer->start();
    }
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

    m_selfAnimating = true;
    m_map->setMargins(QMargins(0, effectiveCenterOffset(), 0, 0));

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

        m_followStartLat = m_currentLocation.latitude;
        m_followStartLon = m_currentLocation.longitude;
        m_followStartBearing = m_map->bearing();
        m_followStartTime = QDateTime::currentMSecsSinceEpoch();

        if (m_targetZoom >= 0) {
            m_selfAnimating = true;
            m_map->setZoom(m_targetZoom);
        } else {
            m_targetZoom = m_map->zoom();
        }
        if (m_targetPitch >= 0) {
            m_selfAnimating = true;
            m_map->setPitch(m_targetPitch);
        } else {
            m_targetPitch = m_map->pitch();
        }

        if (m_overlay) {
            repositionOverlay();
        }

        if (!m_followingPaused && m_animTimer && !m_animTimer->isActive()) {
            m_animTimer->start();
        }
        if (m_resumeTimer) {
            m_resumeTimer->stop();
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

void LocationIndicatorManager::safeSetCoordinate(double lat, double lon)
{
    m_selfAnimating = true;
    m_map->setCoordinate(QMapLibre::Coordinate(lat, lon));
}

void LocationIndicatorManager::safeSetBearing(double bearing)
{
    m_selfAnimating = true;
    m_map->setBearing(bearing);
}

static double smoothstep(double t) {
    t = qBound(0.0, t, 1.0);
    return t * t * (3 - 2 * t);
}

static double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

static double bearingDelta(double from, double to) {
    double delta = to - from;
    if (delta > 180.0) delta -= 360.0;
    if (delta < -180.0) delta += 360.0;
    return delta;
}

void LocationIndicatorManager::onAnimStep()
{
    if (!m_map)
        return;

    if (m_state == State::FixedFollowing && !m_followingPaused) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_followStartTime;
        double progress = qMin(1.0, static_cast<double>(elapsed) / m_animDuration);
        double eased = progress;

        double lat = m_followStartLat + (m_followTargetLat - m_followStartLat) * eased;
        double lon = m_followStartLon + (m_followTargetLon - m_followStartLon) * eased;
        m_selfAnimating = true;
        safeSetCoordinate(lat, lon);

        if (m_targetBearing >= 0) {
            if (m_fixedHeadingMode == FixedHeadingMode::HeadingUp) {
                double delta = bearingDelta(m_followStartBearing, m_targetBearing);
                safeSetBearing(m_followStartBearing + delta * eased);
            } else {
                if (qAbs(m_map->bearing()) > 0.01)
                    safeSetBearing(0.0);
            }
        }
    } else if ((m_state == State::FreeVisible || m_state == State::FixedBrowsing) && m_layerSetup) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_iconStartTime;
        double progress = qMin(1.0, static_cast<double>(elapsed) / m_animDuration);
        double eased = progress;

        m_displayLat = m_iconStartLat + (m_currentLocation.latitude - m_iconStartLat) * eased;
        m_displayLon = m_iconStartLon + (m_currentLocation.longitude - m_iconStartLon) * eased;
        updateSourceToCoordinate(m_displayLat, m_displayLon);
    }
}

bool LocationIndicatorManager::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_parentWidget && event->type() == QEvent::Resize) {
        repositionOverlay();
    }
    return QObject::eventFilter(watched, event);
}

void LocationIndicatorManager::updateOverlayRotation()
{
    if (!m_overlay || m_icon.isNull())
        return;

    const double dpr = QGuiApplication::primaryScreen()
                           ? QGuiApplication::primaryScreen()->devicePixelRatio()
                           : 1.0;
    int scaledW = static_cast<int>(m_icon.width() * dpr);
    int scaledH = static_cast<int>(m_icon.height() * dpr);

    if (m_fixedHeadingMode == FixedHeadingMode::NorthUp && m_currentLocation.heading.has_value()) {
        // Rotate icon to point in heading direction — rotate around center
        QImage scaled = m_icon.scaled(scaledW, scaledH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        // Create a square canvas that fits the rotated image (diagonal)
        int side = qMax(scaledW, scaledH);
        int diag = static_cast<int>(side * 1.5);  // enough for any rotation

        // Paint original image centered on larger canvas
        QImage canvas(diag, diag, QImage::Format_ARGB32);
        canvas.fill(Qt::transparent);
        QPainter p(&canvas);
        p.drawImage((diag - scaledW) / 2, (diag - scaledH) / 2, scaled);
        p.end();

        // Rotate around center of canvas
        QTransform transform;
        transform.translate(diag / 2.0, diag / 2.0);
        transform.rotate(m_currentLocation.heading.value());
        transform.translate(-diag / 2.0, -diag / 2.0);

        QPixmap rotated = QPixmap::fromImage(canvas).transformed(transform, Qt::SmoothTransformation);
        m_overlay->setPixmap(rotated);
        m_overlay->setFixedSize(rotated.size());
    } else {
        // HeadingUp: no rotation
        QImage scaled = m_icon.scaled(scaledW, scaledH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        m_overlay->setPixmap(QPixmap::fromImage(scaled));
        m_overlay->setFixedSize(scaledW, scaledH);
    }
    repositionOverlay();
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
    int eff = effectiveCenterOffset();
    int centerY = eff + (ph - eff) / 2;

    m_overlay->move(centerX - ow / 2, centerY - oh / 2);
    m_overlay->raise();
}




