#ifndef MAPPOLYGON_H
#define MAPPOLYGON_H

#include <QString>
#include <QVector>
#include <QPair>
#include <QColor>

/**
 * @brief 地图多边形数据结构
 *
 * 表示地图上的一个多边形区域。多个多边形可以通过相同的 polygonId 关联为同一组。
 * 每个多边形可以设置填充颜色、透明度、边框颜色、边框宽度和虚实线样式。
 *
 * 坐标使用 (latitude, longitude) 顺序存储，
 * 至少需要 3 个坐标点才能构成有效的多边形。
 *
 * @code
 * // 示例：创建一个多边形
 * MapPolygon poly;
 * poly.id = "poly-001";                    // 多边形唯一标识
 * poly.polygonId = "zone-A";               // 组ID（多边形共享）
 * poly.coordinates = {
 *     {39.9042, 116.4074},                 // (lat, lon) 顺序
 *     {39.9163, 116.3972},
 *     {39.9300, 116.3900},
 *     {39.9042, 116.4074}                  // 闭合点
 * };
 * poly.fillEnabled = true;                 // 启用填充
 * poly.fillColor = QColor("#FF5722");      // 填充颜色
 * poly.fillOpacity = 0.5;                  // 填充透明度
 * poly.strokeColor = QColor("#000000");    // 边框颜色
 * poly.strokeWidth = 2.0;                  // 边框像素宽度
 * poly.strokeDashed = false;               // 实线边框
 * poly.title = "区域A";                    // 显示名称（空=不显示）
 *
 * // 添加到地图
 * mapContainer->addPolygons({poly});
 *
 * // 按组ID控制显隐
 * mapContainer->setVisiblePolygonIds({"zone-A"});
 * @endcode
 *
 * @see MapContainer::addPolygons(), MapContainer::setVisiblePolygonIds()
 */
struct MapPolygon {
    QString id;                                  ///< 多边形唯一标识
    QString polygonId;                           ///< 所属组ID（多边形共享）
    QVector<QPair<double, double>> coordinates;  ///< 坐标点列表 (lat, lon)，至少3个点
    bool fillEnabled = true;                     ///< 是否启用填充
    QColor fillColor;                            ///< 填充颜色
    double fillOpacity = 0.5;                    ///< 填充透明度 0.0-1.0
    QColor strokeColor;                          ///< 边框颜色
    double strokeWidth = 2.0;                    ///< 边框像素宽度
    bool strokeDashed = false;                   ///< 是否虚线边框
    QString title;                               ///< 多边形名称（空=不显示标签）

    /**
     * @brief 验证多边形数据是否有效
     *
     * 检查规则：id 非空、至少 3 个坐标点、坐标在合法范围内。
     * 不检查颜色有效性或边框宽度。
     *
     * @return true 数据有效，false 数据不完整或非法
     */
    bool isValid() const {
        if (id.isEmpty()) return false;
        if (coordinates.size() < 3) return false;
        for (const auto& coord : coordinates) {
            if (coord.first < -90.0 || coord.first > 90.0) return false;
            if (coord.second < -180.0 || coord.second > 180.0) return false;
        }
        return true;
    }
};

#endif // MAPPOLYGON_H
