#include "mapcontainer.h"
#include <QMapLibreWidgets/GLWidget>
#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMapLibre/Types>
#include <QVBoxLayout>
#include <QTouchEvent>
#include <QLineF>
#include <cmath>

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

    setAttribute(Qt::WA_AcceptTouchEvents);
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
    switch (event->type()) {
    case QEvent::TouchBegin: {
        auto *touchEvent = static_cast<QTouchEvent *>(event);
        m_touchActive = true;
        m_touchPointCount = touchEvent->points().count();
        m_lastTouchPoints = touchEvent->points();
        map()->setGestureInProgress(true);
        event->accept();
        return true;
    }
    case QEvent::TouchUpdate: {
        auto *touchEvent = static_cast<QTouchEvent *>(event);
        const auto &points = touchEvent->points();
        m_touchPointCount = points.count();

        if (m_touchPointCount == 1 && m_lastTouchPoints.count() >= 1) {
            QPointF delta = points.first().position() - m_lastTouchPoints.first().position();
            map()->moveBy(delta);
        } else if (m_touchPointCount == 2 && m_lastTouchPoints.count() >= 2) {
            const QPointF &p1 = points.at(0).position();
            const QPointF &p2 = points.at(1).position();
            const QPointF &prevP1 = m_lastTouchPoints.at(0).position();
            const QPointF &prevP2 = m_lastTouchPoints.at(1).position();

            qreal currDist = QLineF(p1, p2).length();
            qreal prevDist = QLineF(prevP1, prevP2).length();
            if (prevDist > 0 && currDist > 0) {
                qreal scaleFactor = currDist / prevDist;
                QPointF center = (p1 + p2) / 2.0;
                map()->scaleBy(scaleFactor, center);
            }

            QLineF currLine(p1, p2);
            QLineF prevLine(prevP1, prevP2);
            qreal angleDelta = currLine.angle() - prevLine.angle();
            if (std::abs(angleDelta) > 0.1) {
                map()->rotateBy(p1, p2);
            }

            QPointF centerDelta = ((p1 + p2) / 2.0) - ((prevP1 + prevP2) / 2.0);
            if (centerDelta.manhattanLength() > 0) {
                map()->moveBy(centerDelta);
            }
        }

        m_lastTouchPoints = points;
        event->accept();
        return true;
    }
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        m_touchActive = false;
        m_touchPointCount = 0;
        m_lastTouchPoints.clear();
        map()->setGestureInProgress(false);
        event->accept();
        return true;
    }
    default:
        return QWidget::event(event);
    }
}

void MapContainer::mousePressEvent(QMouseEvent *event) {
    if (m_touchActive) {
        event->ignore();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MapContainer::mouseMoveEvent(QMouseEvent *event) {
    if (m_touchActive) {
        event->ignore();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void MapContainer::mouseReleaseEvent(QMouseEvent *event) {
    if (m_touchActive) {
        event->ignore();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void MapContainer::wheelEvent(QWheelEvent *event) {
    if (m_touchActive) {
        event->ignore();
        return;
    }
    QWidget::wheelEvent(event);
}
