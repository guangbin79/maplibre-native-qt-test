#ifndef GEOJSONEXPORTER_H
#define GEOJSONEXPORTER_H

#include <QByteArray>
#include <QVector>
#include "mapannotation.h"
#include "maproutesegment.h"
#include "mappolygon.h"

/**
 * @brief GeoJSON 数据导出器（纯内存转换）
 *
 * 将地图要素数据转换为 GeoJSON FeatureCollection 的 QByteArray。
 * 与渲染用 Builder 的区别：
 *   - 每个 MapPolygon 只生成 1 个 Polygon Feature（非渲染用的 fill/outline/label 3个）
 *   - 专注于数据持久化格式，而非 MapLibre 渲染格式
 *   - 坐标遵循 GeoJSON 规范 [lon, lat]
 *   - QColor 转为 "#RRGGBB" 字符串
 */
class GeoJsonExporter {
public:
    /**
     * @brief 将标注列表导出为 GeoJSON FeatureCollection
     *
     * 每个 MapAnnotation 生成一个 Point Feature，坐标顺序遵循 GeoJSON 规范 [lon, lat]。
     * 属性包含：id, title, icon（iconName 的映射）。
     *
     * @param annotations 标注数据列表，每个包含 id/latitude/longitude/title/iconName
     * @return GeoJSON FeatureCollection 的 QByteArray（Compact 格式）
     *
     * @code
     * QVector<MapAnnotation> anns = {
     *     { .id="p1", .latitude=39.9, .longitude=116.4, .title="天安门", .iconName="marker" }
     * };
     * QByteArray geojson = GeoJsonExporter::buildAnnotations(anns);
     * QFile file("annotations.geojson");
     * file.open(QIODevice::WriteOnly);
     * file.write(geojson);
     * @endcode
     *
     * @see GeoJsonImporter::parseAnnotations()
     */
    static QByteArray buildAnnotations(const QVector<MapAnnotation>& annotations);

    /**
     * @brief 将线路段列表导出为 GeoJSON FeatureCollection
     *
     * 每个 MapRouteSegment 生成一个 LineString Feature，坐标顺序 [lon, lat]。
     * 属性包含：id, routeId, color（"#RRGGBB" 格式）, width, dashed。
     * 同一 routeId 的多个段会各自生成独立的 Feature。
     *
     * @param segments 线路段数据列表
     * @return GeoJSON FeatureCollection 的 QByteArray（Compact 格式）
     *
     * @code
     * QVector<MapRouteSegment> segs = {
     *     { .id="s1", .routeId="route-A", .coordinates={{39.9,116.4},{39.92,116.38}},
     *       .color=QColor("#FF5722"), .width=4.0, .dashed=false }
     * };
     * QByteArray geojson = GeoJsonExporter::buildRoutes(segs);
     * @endcode
     *
     * @see GeoJsonImporter::parseRoutes()
     */
    static QByteArray buildRoutes(const QVector<MapRouteSegment>& segments);

    /**
     * @brief 将多边形列表导出为 GeoJSON FeatureCollection
     *
     * 每个 MapPolygon 生成一个 Polygon Feature（仅 1 个，非渲染用的 fill/outline/label 3 个），
     * 坐标顺序 [lon, lat]。属性包含：id, color（"#RRGGBB"）, opacity, title。
     *
     * @param polygons 多边形数据列表
     * @return GeoJSON FeatureCollection 的 QByteArray（Compact 格式）
     *
     * @code
     * QVector<MapPolygon> polys = {
     *     { .id="zone1", .coordinates={{39.9,116.4},{39.92,116.38},{39.91,116.35}},
     *       .color=QColor("#4CAF50"), .opacity=0.4, .title="绿地区域" }
     * };
     * QByteArray geojson = GeoJsonExporter::buildPolygons(polys);
     * @endcode
     *
     * @see GeoJsonImporter::parsePolygons()
     */
    static QByteArray buildPolygons(const QVector<MapPolygon>& polygons);
};

#endif // GEOJSONEXPORTER_H
