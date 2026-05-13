#include "geojsonexporter.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QByteArray GeoJsonExporter::buildAnnotations(const QVector<MapAnnotation>& annotations)
{
    QJsonArray featuresArray;

    for (const auto& ann : annotations) {
        QJsonArray coords;
        coords.append(ann.longitude);
        coords.append(ann.latitude);

        QJsonObject geometry;
        geometry["type"] = "Point";
        geometry["coordinates"] = coords;

        QJsonObject properties;
        properties["id"] = ann.id;
        properties["title"] = ann.title;
        properties["icon"] = ann.iconName;

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

QByteArray GeoJsonExporter::buildRoutes(const QVector<MapRouteSegment>& segments)
{
    QJsonArray featuresArray;

    for (const auto& seg : segments) {
        QJsonArray coordsArray;
        for (const auto& coord : seg.coordinates) {
            QJsonArray point;
            point.append(coord.second);  // lon
            point.append(coord.first);   // lat
            coordsArray.append(point);
        }

        QJsonObject geometry;
        geometry["type"] = "LineString";
        geometry["coordinates"] = coordsArray;

        QJsonObject properties;
        properties["id"] = seg.id;
        properties["routeId"] = seg.routeId;
        properties["color"] = seg.color.name();
        properties["width"] = seg.width;
        properties["lineType"] = seg.dashed ? "dashed" : "solid";
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

QByteArray GeoJsonExporter::buildPolygons(const QVector<MapPolygon>& polygons)
{
    QJsonArray featuresArray;

    for (const auto& poly : polygons) {
        QJsonArray ring;
        for (const auto& coord : poly.coordinates) {
            QJsonArray point;
            point.append(coord.second);  // lon
            point.append(coord.first);   // lat
            ring.append(point);
        }
        if (!poly.coordinates.isEmpty()
            && poly.coordinates.first() != poly.coordinates.last()) {
            QJsonArray closing;
            closing.append(poly.coordinates.first().second);
            closing.append(poly.coordinates.first().first);
            ring.append(closing);
        }

        QJsonArray ringsArray;
        ringsArray.append(ring);

        QJsonObject geometry;
        geometry["type"] = "Polygon";
        geometry["coordinates"] = ringsArray;

        QJsonObject properties;
        properties["id"] = poly.id;
        properties["polygonId"] = poly.polygonId;
        properties["fillEnabled"] = poly.fillEnabled;
        properties["fillColor"] = poly.fillColor.name();
        properties["fillOpacity"] = poly.fillOpacity;
        properties["strokeColor"] = poly.strokeColor.name();
        properties["strokeWidth"] = poly.strokeWidth;
        properties["strokeDashed"] = poly.strokeDashed;
        properties["title"] = poly.title;

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
