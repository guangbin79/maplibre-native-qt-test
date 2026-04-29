#ifndef MAPROUTESEGMENT_H
#define MAPROUTESEGMENT_H

#include <QString>
#include <QVector>
#include <QPair>
#include <QColor>

/**
 * @brief 地图线段数据结构
 *
 * 表示一条线路中的一个段。多条段通过相同的 routeId 关联为同一条逻辑线路。
 * 每个段可以有不同的颜色、宽度和虚实线样式。
 *
 * 坐标使用 (latitude, longitude) 顺序存储，
 * 内部自动转换为 GeoJSON 的 [longitude, latitude] 顺序。
 *
 * @code
 * // 示例：创建一条包含两段的线路
 *
 * // 第一段：实线
 * MapRouteSegment seg1;
 * seg1.id = "seg-001";                // 段唯一标识
 * seg1.routeId = "route-A";           // 线路ID（多段共享）
 * seg1.coordinates = {
 *     {39.9042, 116.4074},            // (lat, lon) 顺序
 *     {39.9163, 116.3972},
 *     {39.9300, 116.3900}
 * };
 * seg1.color = QColor("#FF5722");     // QColor 颜色
 * seg1.width = 4.0;                   // 像素宽度
 * seg1.dashed = false;                // 实线
 * seg1.title = "线路A-段1";           // 显示名称（空=不显示）
 *
 * // 第二段：虚线
 * MapRouteSegment seg2;
 * seg2.id = "seg-002";
 * seg2.routeId = "route-A";           // 相同 routeId = 同一线路
 * seg2.coordinates = {
 *     {39.9300, 116.3900},
 *     {39.9450, 116.3800}
 * };
 * seg2.color = QColor("#FF5722");
 * seg2.width = 4.0;
 * seg2.dashed = true;                 // 虚线
 * seg2.title = "规划中";
 *
 * // 添加到地图
 * mapContainer->addRouteSegments({seg1, seg2});
 *
 * // 按线路ID控制显隐
 * mapContainer->setVisibleRouteIds({"route-A"});
 * @endcode
 *
 * @see MapContainer::addRouteSegments(), MapContainer::setVisibleRouteIds()
 */
struct MapRouteSegment {
    QString id;                                  ///< 段唯一标识
    QString routeId;                             ///< 所属线路ID（多段共享）
    QVector<QPair<double, double>> coordinates;  ///< 坐标点列表 (lat, lon)
    QColor color;                                ///< 线路颜色
    double width = 0.0;                          ///< 像素宽度
    bool dashed = false;                         ///< 是否虚线
    QString title;                               ///< 线路名称（空=不显示标签）

    /**
     * @brief 验证线段数据是否有效
     *
     * 检查规则：id 非空、至少 2 个坐标点、颜色有效、宽度 > 0、坐标在合法范围内。
     *
     * @return true 数据有效，false 数据不完整或非法
     */
    bool isValid() const {
        if (id.isEmpty()) return false;
        if (coordinates.size() < 2) return false;
        if (!color.isValid()) return false;
        if (width <= 0.0) return false;
        for (const auto& coord : coordinates) {
            if (coord.first < -90.0 || coord.first > 90.0) return false;
            if (coord.second < -180.0 || coord.second > 180.0) return false;
        }
        return true;
    }
};

#endif // MAPROUTESEGMENT_H
