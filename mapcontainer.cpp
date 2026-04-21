#include "mapcontainer.h"
#include <QMapLibreWidgets/GLWidget>
#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMapLibre/Types>
#include <QVBoxLayout>

#ifdef Q_OS_ANDROID
#include <QGestureEvent>
#include <QPinchGesture>
#include <cmath>
#endif

MapContainer::MapContainer(const MapConfig &config, QWidget *parent)
    : QWidget(parent)
    , m_glWidget(nullptr)
{
    // Configure settings
    QMapLibre::Settings settings;
    // Coordinate order is (latitude, longitude)
    settings.setDefaultCoordinate(config.defaultCoordinate);
    settings.setDefaultZoom(config.defaultZoom);
    if (!config.styleUrl.isEmpty()) {
        settings.setStyles(QMapLibre::Styles{
            QMapLibre::Style(config.styleUrl, QStringLiteral("HXGIS Day"))
        });
    }

    m_glWidget = new QMapLibre::GLWidget(settings);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_glWidget);

    // QMapLibre::Map uses a single mapChanged(MapChange) signal rather than
    // per-property signals. Region change events cover all camera movements.
    QMapLibre::Map *m = m_glWidget->map();
    connect(m, &QMapLibre::Map::mapChanged, this, [this, m](QMapLibre::Map::MapChange change) {
        if (change != QMapLibre::Map::MapChangeRegionDidChange &&
            change != QMapLibre::Map::MapChangeRegionDidChangeAnimated)
            return;

        if (m->zoom() != m_lastZoom) {
            m_lastZoom = m->zoom();
            emit zoomChanged(m_lastZoom);
        }
        if (m->bearing() != m_lastBearing) {
            m_lastBearing = m->bearing();
            emit bearingChanged(m_lastBearing);
        }
        if (m->pitch() != m_lastPitch) {
            m_lastPitch = m->pitch();
            emit tiltChanged(m_lastPitch);
        }
        auto coord = m->coordinate();
        if (coord.first != m_lastLat || coord.second != m_lastLon) {
            m_lastLat = coord.first;
            m_lastLon = coord.second;
            emit centerChanged(coord.first, coord.second);
        }
    });

    m_lastZoom = settings.defaultZoom();
    m_lastBearing = 0.0;
    m_lastPitch = 0.0;
    m_lastLat = settings.defaultCoordinate().first;
    m_lastLon = settings.defaultCoordinate().second;

#ifdef Q_OS_ANDROID
    grabGesture(Qt::PinchGesture);
#endif
}

void MapContainer::setStyle(const QString &styleUrl) {
    m_glWidget->map()->setStyleUrl(styleUrl);
}

void MapContainer::setCenter(double lat, double lon) {
    m_glWidget->map()->setCoordinate(QMapLibre::Coordinate(lat, lon));
}

void MapContainer::setZoom(double zoom) {
    m_glWidget->map()->setZoom(zoom);
}

void MapContainer::setBearing(double bearing) {
    m_glWidget->map()->setBearing(bearing);
}

void MapContainer::setPitch(double pitch) {
    m_glWidget->map()->setPitch(pitch);
}

QMapLibre::Map *MapContainer::map() const {
    return m_glWidget->map();
}

bool MapContainer::event(QEvent *event) {
#ifdef Q_OS_ANDROID
    if (event->type() == QEvent::Gesture) {
        auto *gestureEvent = static_cast<QGestureEvent*>(event);
        if (auto *pinch = static_cast<QPinchGesture*>(gestureEvent->gesture(Qt::PinchGesture))) {
            handlePinchGesture(pinch);
            return true;
        }
    }
#endif
    return QWidget::event(event);
}

#ifdef Q_OS_ANDROID
void MapContainer::handlePinchGesture(QPinchGesture *gesture) {
    QMapLibre::Map *m = m_glWidget->map();

    if (gesture->state() == Qt::GestureStarted) {
        m_gestureActive = true;
        m_startZoom = m->zoom();
        m_startBearing = m->bearing();
    }

    if (gesture->state() == Qt::GestureUpdated || gesture->state() == Qt::GestureStarted) {
        // Scale (zoom)
        if (gesture->changeFlags() & QPinchGesture::ScaleFactorChanged) {
            double newZoom = m_startZoom + std::log2(gesture->totalScaleFactor());
            m->setZoom(newZoom);
        }

        // Rotation (bearing)
        if (gesture->changeFlags() & QPinchGesture::RotationAngleChanged) {
            double newBearing = m_startBearing + gesture->totalRotationAngle();
            m->setBearing(newBearing);
        }
    }

    if (gesture->state() == Qt::GestureFinished || gesture->state() == Qt::GestureCanceled) {
        m_gestureActive = false;
    }
}
#endif
