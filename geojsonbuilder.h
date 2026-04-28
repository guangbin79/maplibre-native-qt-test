/**
 * @file geojsonbuilder.h
 * @brief GeoJSON 构建器
 *
 * 将 MapAnnotation 列表转换为 GeoJSON FeatureCollection。
 * 坐标顺序严格遵循 GeoJSON 规范：[longitude, latitude]。
 */

#ifndef GEOJSONBUILDER_H
#define GEOJSONBUILDER_H

#include <QByteArray>
#include <QVector>
#include "mapannotation.h"

/**
 * @brief GeoJSON 构建器
 *
 * 将 MapAnnotation 列表转换为 GeoJSON FeatureCollection。
 * 坐标顺序严格遵循 GeoJSON 规范：[longitude, latitude]。
 */
class GeoJsonBuilder {
public:
    /**
     * @brief 构建 GeoJSON FeatureCollection
     * @param annotations 标注列表
     * @return GeoJSON 格式的 JSON 字节串
     */
    static QByteArray buildFeatureCollection(const QVector<MapAnnotation>& annotations);
};

#endif // GEOJSONBUILDER_H
