#ifndef ROUTEGEOJSONBUILDER_H
#define ROUTEGEOJSONBUILDER_H

#include <QByteArray>
#include <QVector>
#include "maproutesegment.h"

/**
 * @brief 线路段 GeoJSON 构建器
 *
 * 将 MapRouteSegment 列表转换为 GeoJSON FeatureCollection。
 * 每段生成一个 LineString Feature，包含样式属性用于 MapLibre data-driven 渲染。
 *
 * 坐标转换：MapRouteSegment 使用 (lat, lon)，GeoJSON 使用 [lon, lat]。
 * 颜色转换：QColor 通过 name() 转为 "#RRGGBB" 字符串。
 *
 * 每个 Feature 的 properties 包含：
 *   - id (string):        段唯一标识
 *   - routeId (string):   所属线路ID
 *   - lineType (string):  "solid" 或 "dashed"
 *   - color (string):     线路颜色 "#rrggbb"
 *   - width (double):     像素宽度
 *   - title (string):     线路名称
 *   - showArrows (bool):  是否显示方向箭头
 *   - arrowSize (double): 箭头大小缩放因子
 *   - arrowColor (string): 箭头颜色 "#rrggbb"（通过 effectiveArrowColor() 解析）
 */
class RouteGeoJsonBuilder {
public:
    static QByteArray buildFeatureCollection(
        const QVector<MapRouteSegment>& segments);
};

#endif // ROUTEGEOJSONBUILDER_H
