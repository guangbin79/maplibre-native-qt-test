#include "routegeojsonbuilder.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QByteArray RouteGeoJsonBuilder::buildFeatureCollection(
    const QVector<MapRouteSegment>& segments)
{
    QJsonArray featuresArray;
    for (const auto& seg : segments) {
        QJsonArray coordsArray;
        for (const auto& coord : seg.coordinates) {
            QJsonArray point;
            point.append(coord.second);  // lon (GeoJSON: [lon, lat])
            point.append(coord.first);   // lat
            coordsArray.append(point);
        }

        QJsonObject geometry;
        geometry["type"] = "LineString";
        geometry["coordinates"] = coordsArray;

        QJsonObject properties;
        properties["id"] = seg.id;
        properties["routeId"] = seg.routeId;
        properties["lineType"] = seg.dashed ? "dashed" : "solid";
        properties["color"] = seg.color.name();
        properties["width"] = seg.width;
        properties["title"] = seg.title;

        QJsonObject feature;
        feature["type"] = "Feature";
        feature["geometry"] = geometry;
        feature["properties"] = properties;

        featuresArray.append(feature);
    }

    QJsonObject root;
    root["type"] = "FeatureCollection";
    root["features"] = featuresArray;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}
