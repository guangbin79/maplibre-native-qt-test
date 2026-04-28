#ifndef MAPROUTESEGMENT_H
#define MAPROUTESEGMENT_H

#include <QString>
#include <QVector>
#include <QPair>
#include <QColor>

/**
 * @brief 地图线路段数据结构
 *
 * 表示一条线路中的一个段。多条段通过相同的 routeId 关联为同一线路。
 * 每段可独立设置颜色、宽度、虚实线样式。
 *
 * 坐标使用 (latitude, longitude) 顺序存储，
 * 写入 GeoJSON 时需转换为 [longitude, latitude] 顺序。
 */
struct MapRouteSegment {
    QString id;                                  ///< 段唯一标识
    QString routeId;                             ///< 所属线路ID（多段共享）
    QVector<QPair<double, double>> coordinates;  ///< 坐标点列表 (lat, lon)
    QColor color;                                ///< 线路颜色
    double width = 0.0;                          ///< 像素宽度
    bool dashed = false;                         ///< 是否虚线
    QString title;                               ///< 线路名称（空=不显示标签）

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
