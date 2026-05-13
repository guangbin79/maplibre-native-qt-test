#ifndef ROUTEMANAGER_H
#define ROUTEMANAGER_H

#include <QObject>
#include <QVector>
#include <QStringList>
#include <QMapLibre/Types>
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

    /**
     * @brief 获取所有线路段数据
     *
     * 返回当前管理器中所有线路段的副本。可用于导出、序列化或遍历查询。
     *
     * @return 线路段数据的 QVector 副本
     *
     * @code
     * auto segs = manager->segments();
     * QByteArray geojson = GeoJsonExporter::buildRoutes(segs);
     * @endcode
     *
     * @see setSegments(), GeoJsonExporter::buildRoutes()
     */
    QVector<MapRouteSegment> segments() const;

    bool boundingBoxForRoute(const QString& routeId,
                             QMapLibre::Coordinate& sw,
                             QMapLibre::Coordinate& ne) const;

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
