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
#include "cameraanimationmath.h"


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

        if (!m_map || m_state != State::FixedBrowsing)
            return;

        // Start resume animation: map slides from drag position back to GPS.
        // Symbol Layer stays visible during animation (icon pinned at GPS coordinate).
        // Overlay shown only AFTER animation completes (in onAnimStep).
        m_state = State::FixedFollowing;
        emit followingPausedChanged(false);
        m_resumeAnimating = true;

        // Pin Symbol Layer to exact GPS position
        m_displayLat = m_currentLocation.latitude;
        m_displayLon = m_currentLocation.longitude;
        updateSourceToCoordinate(m_displayLat, m_displayLon);

        // Restore Fixed mode margins
        {
            const QSignalBlocker blocker(m_map);
            m_map->setMargins(QMargins(0, effectiveCenterOffset(), 0, 0));

            // Restore zoom/pitch
            if (m_targetZoom >= 0) {
                m_map->setZoom(m_targetZoom);
            }
            if (m_targetPitch >= 0) {
                m_map->setPitch(m_targetPitch);
            }
        }

        // Record animation start = current map center (drag position)
        auto coord = m_map->coordinate();
        m_followStartLat = coord.first;
        m_followStartLon = coord.second;
        m_followStartBearing = m_map->bearing();
        m_followStartTime = QDateTime::currentMSecsSinceEpoch();
        m_followTargetLat = m_currentLocation.latitude;
        m_followTargetLon = m_currentLocation.longitude;

        // Start animation timer
        if (m_animTimer && !m_animTimer->isActive())
            m_animTimer->start();
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
                        transitionToBrowsing();
                    } else if (change == QMapLibre::Map::MapChangeRegionDidChange
                               || change == QMapLibre::Map::MapChangeRegionDidChangeAnimated) {
                        if (m_state == State::FixedBrowsing && m_resumeTimer)
                            m_resumeTimer->start();
                    }
                });
    }
}

void LocationIndicatorManager::setLocation(const LocationData& data)
{
    bool coordsChanged = (data.latitude != m_currentLocation.latitude
                          || data.longitude != m_currentLocation.longitude);
    bool headingChanged = (data.heading != m_currentLocation.heading);
    bool headingPresenceChanged = (data.heading.has_value() != m_currentLocation.heading.has_value());

    m_currentLocation = data;

    if (!m_displayInitialized) {
        m_displayLat = data.latitude;
        m_displayLon = data.longitude;
        m_displayInitialized = true;
    }

    if (coordsChanged || headingChanged)
        emit locationChanged(m_currentLocation);

    if (headingPresenceChanged && m_layerSetup && m_map)
        updateIconAlignment();

    if (!m_ready)
        return;

    if (m_state != State::Hidden) {
        ensureLayerSetup();

        if (m_state == State::FreeVisible || m_state == State::FixedBrowsing) {
            if (coordsChanged && m_animTimer && !m_animTimer->isActive()) {
                m_animTimer->start();
            }
            else if (!m_animTimer || !m_animTimer->isActive())
                updateSourceToCoordinate(m_displayLat, m_displayLon);
        } else {
            if (!m_resumeAnimating)
                rebuildSource();
        }
    }

    if (data.heading.has_value()) {
        m_rotation = data.heading.value();
        if (m_layerSetup && m_map) {
            m_map->setLayoutProperty("location-indicator-layer",
                                      "icon-rotate", m_rotation);
        }

        if (m_state == State::FixedFollowing && m_overlay && headingChanged
            && m_fixedHeadingMode == FixedHeadingMode::NorthUp) {
            // Start smoothed rotation: from currently-displayed angle (NOT m_rotation
            // which is raw heading for Symbol Layer) to new heading target.
            m_overlayRotStart = m_overlayCurrentAngle;  // E3: start from current displayed angle
            m_overlayRotTarget = data.heading.value();
            m_overlayRotStartTime = QDateTime::currentMSecsSinceEpoch();
            // NOTE: NOT calling updateOverlayRotation — onAnimStep interpolates next frame
        }
    }

    // Store follow targets for onAnimStep
    m_followTargetLat = data.latitude;
    m_followTargetLon = data.longitude;
    if (data.heading.has_value()) {
        m_targetBearing = data.heading.value();
    } else {
        m_targetBearing = -1.0;
    }

    // Record animation start positions for time-deadline interpolation
    if (m_map && m_state == State::FixedFollowing && !m_followingPaused && !m_resumeAnimating) {
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
            if (m_map->bearing() != 0.0) {
                {
                    const QSignalBlocker blocker(m_map);
                    m_map->setBearing(0.0);
                }
            }
            emit followBearingChanged(0.0);
        }
    }

    if (m_overlay && m_visible && m_mode == LocationMode::Fixed && m_state == State::FixedFollowing) {
        updateOverlayRotation();
        // Cancel any in-flight animation + sync state to snap angle (G3, E3)
        const double snapAngle = (m_fixedHeadingMode == FixedHeadingMode::NorthUp
                                  && m_currentLocation.heading.has_value())
                                 ? m_currentLocation.heading.value() : 0.0;
        m_overlayRotStart = m_overlayRotTarget = m_overlayCurrentAngle = snapAngle;
        m_overlayRotStartTime = QDateTime::currentMSecsSinceEpoch();
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
            // Cancel any in-flight animation + sync state to snap angle (G3, E3)
            const double snapAngle = (m_fixedHeadingMode == FixedHeadingMode::NorthUp
                                      && m_currentLocation.heading.has_value())
                                     ? m_currentLocation.heading.value() : 0.0;
            m_overlayRotStart = m_overlayRotTarget = m_overlayCurrentAngle = snapAngle;
            m_overlayRotStartTime = QDateTime::currentMSecsSinceEpoch();
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

    if (m_animTimer)
        m_animTimer->stop();

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
        const QSignalBlocker blocker(m_map);
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
        const QSignalBlocker blocker(m_map);
        m_map->setZoom(zoom);
    }
}

void LocationIndicatorManager::setPitch(double pitch)
{
    m_targetPitch = pitch;
    if (m_state == State::FixedFollowing && m_map) {
        const QSignalBlocker blocker(m_map);
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
    transitionToBrowsing();
    if (m_state == State::FixedBrowsing && m_resumeTimer)
        m_resumeTimer->start();
}

LocationIndicatorManager::State LocationIndicatorManager::state() const
{
    return m_state;
}

void LocationIndicatorManager::transitionToBrowsing()
{
    if (m_state != State::FixedFollowing) return;
    m_state = State::FixedBrowsing;
    m_followingPaused = true;
    if (m_animTimer) m_animTimer->stop();
    if (m_resumeTimer) {
        m_resumeTimer->setInterval(m_fixedTouchResumeTimeout);
        m_resumeTimer->start();
    }
    if (m_overlay) m_overlay->hide();
    if (m_layerSetup && m_map) {
        m_displayLat = m_currentLocation.latitude;
        m_displayLon = m_currentLocation.longitude;
        updateSourceToCoordinate(m_displayLat, m_displayLon);
        m_map->setLayoutProperty("location-indicator-layer", "visibility", "visible");
    }
    emit followingPausedChanged(true);
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
                             "visibility", "none");

    m_layerSetup = true;
    updateIconAlignment();

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

    if (m_state == State::FixedFollowing) {
        ensureLayerSetup();
        if (m_layerSetup) {
            rebuildSource();
            m_map->setLayoutProperty("location-indicator-layer",
                                     "visibility", "none");
        }

        {
            const QSignalBlocker blocker(m_map);
            m_map->setMargins(QMargins(0, effectiveCenterOffset(), 0, 0));

            QMapLibre::CameraOptions options;
            options.center = QVariant::fromValue(
                QMapLibre::Coordinate(m_currentLocation.latitude,
                                      m_currentLocation.longitude));
            m_map->jumpTo(options);

            if (m_targetZoom >= 0) {
                m_map->setZoom(m_targetZoom);
            } else {
                m_targetZoom = m_map->zoom();
            }
            if (m_targetPitch >= 0) {
                m_map->setPitch(m_targetPitch);
            } else {
                m_targetPitch = m_map->pitch();
            }
        }

        m_followStartLat = m_currentLocation.latitude;
        m_followStartLon = m_currentLocation.longitude;
        m_followStartBearing = m_map->bearing();
        m_followStartTime = QDateTime::currentMSecsSinceEpoch();

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
    const QSignalBlocker blocker(m_map);
    m_map->setCoordinate(QMapLibre::Coordinate(lat, lon));
}

void LocationIndicatorManager::safeSetBearing(double bearing)
{
    const QSignalBlocker blocker(m_map);
    m_map->setBearing(bearing);
}

static double bearingDelta(double from, double to) {
    double delta = to - from;
    if (delta > 180.0) delta -= 360.0;
    if (delta < -180.0) delta += 360.0;
    return delta;
}

LocationIndicatorManager::FollowingFrame LocationIndicatorManager::computeFollowingFrame(qint64 now) const
{
    qint64 elapsed = now - m_followStartTime;
    int duration = m_resumeAnimating ? RESUME_ANIM_DURATION : m_animDuration;
    double progress = qMin(1.0, static_cast<double>(elapsed) / duration);
    double eased = progress;

    double lat = m_followStartLat + (m_followTargetLat - m_followStartLat) * eased;
    double lon = m_followStartLon + (m_followTargetLon - m_followStartLon) * eased;

    return {lat, lon, progress, progress >= 1.0};
}

LocationIndicatorManager::IconFrame LocationIndicatorManager::computeIconFrame(qint64 now) const
{
    qint64 elapsed = now - m_iconStartTime;
    double progress = qMin(1.0, static_cast<double>(elapsed) / m_animDuration);
    double eased = progress;

    double lat = m_iconStartLat + (m_currentLocation.latitude - m_iconStartLat) * eased;
    double lon = m_iconStartLon + (m_currentLocation.longitude - m_iconStartLon) * eased;

    return {lat, lon, progress, progress >= 1.0};
}

void LocationIndicatorManager::maybeEmitVisualLocation(double lat, double lon)
{
    if (m_visualLocationInitialized
        && qAbs(lat - m_lastVisualLat) < 1e-9
        && qAbs(lon - m_lastVisualLon) < 1e-9)
        return;  // identical value — suppress spam
    m_lastVisualLat = lat;
    m_lastVisualLon = lon;
    m_visualLocationInitialized = true;
    emit visualLocationChanged(lat, lon);
}

void LocationIndicatorManager::onAnimStep()
{
    if (!m_map)
        return;

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (m_state == State::FixedFollowing && !m_followingPaused) {
        auto frame = computeFollowingFrame(now);
        safeSetCoordinate(frame.lat, frame.lon);
        maybeEmitVisualLocation(frame.lat, frame.lon);

        if (m_targetBearing >= 0) {
            if (m_fixedHeadingMode == FixedHeadingMode::HeadingUp) {
                double delta = bearingDelta(m_followStartBearing, m_targetBearing);
                safeSetBearing(m_followStartBearing + delta * frame.progress);
                emit followBearingChanged(m_map->bearing());
            } else {
                if (qAbs(m_map->bearing()) > 0.01) {
                    safeSetBearing(0.0);
                    emit followBearingChanged(0.0);
                }
            }
        }

        // Resume animation complete — switch from Symbol Layer to overlay
        if (m_resumeAnimating && frame.complete) {
            // Reset animation start to current GPS position for seamless
            // handoff to normal following (prevents progress jump from
            // 300/300=1.0 back to 300/1200=0.25)
            m_followStartLat = m_followTargetLat;
            m_followStartLon = m_followTargetLon;
            m_followStartBearing = m_map->bearing();
            m_followStartTime = QDateTime::currentMSecsSinceEpoch();
            m_resumeAnimating = false;
            if (m_overlay) {
                m_overlay->show();
                m_overlay->raise();
                repositionOverlay();
                // E7 fix: resume-after-browse must not show stale rotation
                const double snapAngle = m_currentLocation.heading.value_or(0.0);
                m_overlayRotStart = m_overlayRotTarget = m_overlayCurrentAngle = snapAngle;
                m_overlayRotStartTime = QDateTime::currentMSecsSinceEpoch();
                updateOverlayRotation(snapAngle);
            }
            if (m_layerSetup && m_map) {
                m_map->setLayoutProperty("location-indicator-layer",
                                          "visibility", "none");
            }
        }
        // --- NorthUp overlay smoothed rotation interpolation (independent time variable, G5) ---
        if (m_fixedHeadingMode == FixedHeadingMode::NorthUp && m_overlay
            && m_overlayRotStart != m_overlayRotTarget) {
            const qint64 rotElapsed = now - m_overlayRotStartTime;
            const double rotProgress = qMin(1.0, static_cast<double>(rotElapsed)
                                                / OVERLAY_ROTATION_DURATION);
            const double rotEased = CameraMath::easeInOutQuad(rotProgress);
            const double interpolated = CameraMath::interpolateAngle(
                m_overlayRotStart, m_overlayRotTarget, rotEased);
            m_overlayCurrentAngle = interpolated;
            updateOverlayRotation(interpolated);
            if (rotProgress >= 1.0) {
                // Animation complete — lock start=target to stop ticking
                m_overlayRotStart = m_overlayRotTarget;
            }
        }
    } else if ((m_state == State::FreeVisible || m_state == State::FixedBrowsing) && m_layerSetup) {
        auto frame = computeIconFrame(now);
        m_displayLat = frame.lat;
        m_displayLon = frame.lon;
        updateSourceToCoordinate(m_displayLat, m_displayLon);
        maybeEmitVisualLocation(frame.lat, frame.lon);
    }
}

bool LocationIndicatorManager::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_parentWidget && event->type() == QEvent::Resize) {
        repositionOverlay();
    }
    return QObject::eventFilter(watched, event);
}

void LocationIndicatorManager::updateOverlayRotation(double angle)
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
        const double angleToUse = std::isnan(angle)
            ? m_currentLocation.heading.value_or(0.0)
            : angle;
        transform.rotate(angleToUse);
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

void LocationIndicatorManager::updateIconAlignment()
{
    if (!m_layerSetup || !m_map)
        return;

    if (m_currentLocation.heading.has_value()) {
        m_map->setLayoutProperty("location-indicator-layer", "icon-rotation-alignment", "map");
        m_map->setLayoutProperty("location-indicator-layer", "icon-pitch-alignment", "map");
        m_map->setLayoutProperty("location-indicator-layer", "icon-rotate", m_currentLocation.heading.value());
    } else {
        m_map->setLayoutProperty("location-indicator-layer", "icon-rotation-alignment", "viewport");
        m_map->setLayoutProperty("location-indicator-layer", "icon-pitch-alignment", "viewport");
        m_map->setLayoutProperty("location-indicator-layer", "icon-rotate", 0.0);
    }
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




