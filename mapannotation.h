/**
 * @file mapannotation.h
 * @brief 地图标注数据结构
 *
 * @section 概述
 * MapAnnotation 表示地图上的一个标注点，包含坐标、文字和图标引用。
 * 轻量级纯数据结构，无网络、无 JSON 解析、无图像依赖。
 *
 * @section 使用示例
 * @code
 * MapAnnotation ann;
 * ann.id = "poi-001";
 * ann.latitude = 39.9042;
 * ann.longitude = 116.4074;
 * ann.title = "北京天安门";
 * ann.iconName = "marker";
 * if (ann.isValid()) {
 *     // 添加到地图图层
 * }
 * @endcode
 */

#ifndef MAPANNOTATION_H
#define MAPANNOTATION_H

#include <QString>

/**
 * @brief 地图标注数据结构
 *
 * 表示地图上的一个标注点，包含坐标、文字和图标引用。
 */
struct MapAnnotation {
    QString id;              ///< 标注唯一标识
    double latitude = 0.0;   ///< 纬度 [-90, 90]
    double longitude = 0.0;  ///< 经度 [-180, 180]
    QString title;           ///< 标注文字
    QString iconName;        ///< 图标名称，对应 icons map 中的 key

    /**
     * @brief 验证标注数据是否有效
     * @return true 如果 id 非空、纬度 ∈ [-90,90]、经度 ∈ [-180,180]
     */
    bool isValid() const {
        return !id.isEmpty()
            && latitude >= -90.0 && latitude <= 90.0
            && longitude >= -180.0 && longitude <= 180.0;
    }
};

#endif // MAPANNOTATION_H
