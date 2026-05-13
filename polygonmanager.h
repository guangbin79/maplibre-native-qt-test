#ifndef POLYGONMANAGER_H
#define POLYGONMANAGER_H

#include <QObject>
#include <QVector>
#include <QStringList>
#include <QMapLibre/Types>
#include "mappolygon.h"

namespace QMapLibre { class Map; }

class PolygonManager : public QObject {
    Q_OBJECT

public:
    explicit PolygonManager(QMapLibre::Map* map, QObject* parent = nullptr);

    void setMapReady(bool ready);

    void setPolygons(const QVector<MapPolygon>& polygons);
    void clearPolygons();

    void addPolygon(const MapPolygon& polygon);
    void addPolygons(const QVector<MapPolygon>& polygons);

    void removePolygon(const QString& id);
    void removePolygons(const QStringList& ids);

    void setVisiblePolygonIds(const QStringList& polygonIds);
    void showAllPolygons();
    void hideAllPolygons();

    QStringList allPolygonIds() const;
    QStringList visiblePolygonIds() const;

    /**
     * @brief 获取所有多边形数据
     *
     * 返回当前管理器中所有多边形的副本。可用于导出、序列化或遍历查询。
     *
     * @return 多边形数据的 QVector 副本
     *
     * @code
     * auto polys = manager->polygons();
     * QByteArray geojson = GeoJsonExporter::buildPolygons(polys);
     * @endcode
     *
     * @see setPolygons(), GeoJsonExporter::buildPolygons()
     */
    QVector<MapPolygon> polygons() const;

    bool boundingBoxForPolygon(const QString& polygonId,
                                QMapLibre::Coordinate& sw,
                                QMapLibre::Coordinate& ne) const;

private:
    void ensureLayerSetup();
    void rebuildSource();
    void updateFilter();

    QMapLibre::Map* m_map;
    QVector<MapPolygon> m_polygons;
    QStringList m_visiblePolygonIds;
    bool m_ready = false;
    bool m_layerSetup = false;
};

#endif
