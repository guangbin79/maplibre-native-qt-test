#ifndef GEOJSONIMPORTER_H
#define GEOJSONIMPORTER_H

#include <QByteArray>
#include <QVector>
#include "mapannotation.h"
#include "maproutesegment.h"
#include "mappolygon.h"

/**
 * @brief GeoJSON 数据导入器（纯内存转换）
 *
 * 从 GeoJSON QByteArray 解析地图要素数据。
 * 是 GeoJsonExporter 的逆操作：parse(build(data)) == data
 *
 * @section 输入格式要求
 * - 必须是合法的 GeoJSON FeatureCollection（JSON 格式）
 * - 每个 Feature 需包含 type、geometry、properties 字段
 * - 坐标顺序遵循 GeoJSON 规范 [lon, lat]
 *
 * @section 容错策略
 * - 缺少可选属性时使用默认值，不报错
 * - 无效 Feature（缺少 geometry/properties）跳过并继续
 * - 空 FeatureCollection 返回空 QVector（ok=true）
 * - 非法 JSON 或非 FeatureCollection 类型时 ok=false，返回空 QVector
 *
 * @code
 * QByteArray data = file.readAll();
 * bool ok = false;
 * auto anns = GeoJsonImporter::parseAnnotations(data, &ok);
 * if (!ok) {
 *     qWarning() << "GeoJSON 解析失败";
 * }
 * @endcode
 *
 * @see GeoJsonExporter
 */
class GeoJsonImporter {
public:
    /**
     * @brief 从 GeoJSON 解析标注列表
     *
     * 解析 FeatureCollection 中所有 Point 类型的 Feature，转换为 MapAnnotation。
     * 坐标从 GeoJSON [lon, lat] 转为 MapAnnotation 的 (latitude, longitude)。
     *
     * 属性映射：
     * - id → MapAnnotation::id（缺少时生成序号）
     * - title → MapAnnotation::title（缺少时为空）
     * - icon → MapAnnotation::iconName（缺少时为空）
     *
     * @param geojson GeoJSON FeatureCollection 的原始字节
     * @param ok 可选的成功状态指针，解析成功设为 true，失败设为 false
     * @return 解析出的 MapAnnotation 列表，解析失败时返回空列表
     *
     * @code
     * QByteArray data = R"({"type":"FeatureCollection","features":[
     *   {"type":"Feature","geometry":{"type":"Point","coordinates":[116.4,39.9]},
     *    "properties":{"id":"p1","title":"天安门","icon":"marker"}}
     * ]})";
     * bool ok = false;
     * auto anns = GeoJsonImporter::parseAnnotations(data, &ok);
     * // anns[0].latitude == 39.9, anns[0].longitude == 116.4
     * @endcode
     *
     * @see GeoJsonExporter::buildAnnotations()
     */
    static QVector<MapAnnotation> parseAnnotations(const QByteArray& geojson, bool* ok = nullptr);

    /**
     * @brief 从 GeoJSON 解析线路段列表
     *
     * 解析 FeatureCollection 中所有 LineString 类型的 Feature，转换为 MapRouteSegment。
     * 坐标从 GeoJSON [lon, lat] 转为 MapRouteSegment 的 (latitude, longitude) 对。
     * QColor 从 "#RRGGBB" 字符串恢复。
     *
     * 属性映射：
     * - id → MapRouteSegment::id
     * - routeId → MapRouteSegment::routeId
     * - color → MapRouteSegment::color（默认灰色）
     * - width → MapRouteSegment::width（默认 3.0）
     * - dashed → MapRouteSegment::dashed（默认 false）
     *
     * @param geojson GeoJSON FeatureCollection 的原始字节
     * @param ok 可选的成功状态指针，解析成功设为 true，失败设为 false
     * @return 解析出的 MapRouteSegment 列表，解析失败时返回空列表
     *
     * @code
     * QByteArray data = file.readAll();
     * bool ok = false;
     * auto segs = GeoJsonImporter::parseRoutes(data, &ok);
     * if (ok) {
     *     mapContainer->setRoutes(segs);
     * }
     * @endcode
     *
     * @see GeoJsonExporter::buildRoutes()
     */
    static QVector<MapRouteSegment> parseRoutes(const QByteArray& geojson, bool* ok = nullptr);

    /**
     * @brief 从 GeoJSON 解析多边形列表
     *
     * 解析 FeatureCollection 中所有 Polygon 类型的 Feature，转换为 MapPolygon。
     * 坐标从 GeoJSON [lon, lat] 转为 MapPolygon 的 (latitude, longitude) 对。
     * 仅使用外环（rings[0]），忽略内环（孔洞）。
     *
     * 属性映射：
     * - id → MapPolygon::id
     * - color → MapPolygon::color（默认蓝色）
     * - opacity → MapPolygon::opacity（默认 0.4）
     * - title → MapPolygon::title（默认空）
     *
     * @param geojson GeoJSON FeatureCollection 的原始字节
     * @param ok 可选的成功状态指针，解析成功设为 true，失败设为 false
     * @return 解析出的 MapPolygon 列表，解析失败时返回空列表
     *
     * @code
     * QByteArray data = file.readAll();
     * bool ok = false;
     * auto polys = GeoJsonImporter::parsePolygons(data, &ok);
     * if (ok) {
     *     mapContainer->setPolygons(polys);
     * }
     * @endcode
     *
     * @see GeoJsonExporter::buildPolygons()
     */
    static QVector<MapPolygon> parsePolygons(const QByteArray& geojson, bool* ok = nullptr);
};

#endif // GEOJSONIMPORTER_H
