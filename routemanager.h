#ifndef ROUTEMANAGER_H
#define ROUTEMANAGER_H

#include <QObject>
#include <QVector>
#include <QStringList>
#include "maproutesegment.h"

namespace QMapLibre { class Map; }

class RouteManager : public QObject {
    Q_OBJECT

public:
    explicit RouteManager(QMapLibre::Map* map, QObject* parent = nullptr);

    void setMapReady(bool ready);

    void setSegments(const QVector<MapRouteSegment>& segments);
    void clearSegments();

    void addRouteSegment(const MapRouteSegment& segment);
    void addRouteSegments(const QVector<MapRouteSegment>& segments);
    void removeRouteSegment(const QString& id);
    void removeRouteSegments(const QStringList& ids);

    void setVisibleRouteIds(const QStringList& routeIds);
    void showAllRoutes();
    void hideAllRoutes();

    QStringList allRouteIds() const;
    QStringList visibleRouteIds() const;

private:
    void ensureLayerSetup();
    void rebuildSource();
    void updateFilter();

    QMapLibre::Map* m_map;
    QVector<MapRouteSegment> m_segments;
    QStringList m_visibleRouteIds;
    bool m_ready = false;
    bool m_layerSetup = false;
};

#endif
