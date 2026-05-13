#include "polygongeojsonbuilder.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static QJsonArray buildRingCoords(const QVector<QPair<double, double>>& coordinates)
{
    QJsonArray ring;
    for (const auto& coord : coordinates) {
        QJsonArray point;
        point.append(coord.second);
        point.append(coord.first);
        ring.append(point);
    }
    if (!coordinates.isEmpty()
        && coordinates.first() != coordinates.last()) {
        QJsonArray closing;
        closing.append(coordinates.first().second);
        closing.append(coordinates.first().first);
        ring.append(closing);
    }
    return ring;
}

static QJsonObject buildPolygonFeature(const MapPolygon& poly, const QJsonArray& ring)
{
    QJsonArray ringsArray;
    ringsArray.append(ring);

    QJsonObject geometry;
    geometry["type"] = "Polygon";
    geometry["coordinates"] = ringsArray;

    QJsonObject properties;
    properties["id"] = poly.id;
    properties["polygonId"] = poly.polygonId;
    properties["fillEnabled"] = poly.fillEnabled ? "true" : "false";
    properties["fillColor"] = poly.fillColor.name();
    properties["fillOpacity"] = poly.fillOpacity;
    properties["strokeColor"] = poly.strokeColor.name();
    properties["strokeWidth"] = poly.strokeWidth;
    properties["strokeType"] = poly.strokeDashed ? "dashed" : "solid";
    properties["geometryType"] = "fill";

    QJsonObject feature;
    feature["type"] = "Feature";
    feature["geometry"] = geometry;
    feature["properties"] = properties;
    return feature;
}

static QJsonObject buildLineStringFeature(const MapPolygon& poly, const QJsonArray& ring)
{
    QJsonObject geometry;
    geometry["type"] = "LineString";
    geometry["coordinates"] = ring;

    QJsonObject properties;
    properties["id"] = poly.id;
    properties["polygonId"] = poly.polygonId;
    properties["strokeColor"] = poly.strokeColor.name();
    properties["strokeWidth"] = poly.strokeWidth;
    properties["strokeType"] = poly.strokeDashed ? "dashed" : "solid";
    properties["geometryType"] = "outline";

    QJsonObject feature;
    feature["type"] = "Feature";
    feature["geometry"] = geometry;
    feature["properties"] = properties;
    return feature;
}

static QJsonObject buildPointFeature(const MapPolygon& poly)
{
    double sumLat = 0.0;
    double sumLon = 0.0;
    for (const auto& coord : poly.coordinates) {
        sumLat += coord.first;
        sumLon += coord.second;
    }
    double centroidLat = sumLat / poly.coordinates.size();
    double centroidLon = sumLon / poly.coordinates.size();

    QJsonArray centroid;
    centroid.append(centroidLon);
    centroid.append(centroidLat);

    QJsonObject geometry;
    geometry["type"] = "Point";
    geometry["coordinates"] = centroid;

    QJsonObject properties;
    properties["id"] = poly.id;
    properties["polygonId"] = poly.polygonId;
    properties["title"] = poly.title;
    properties["geometryType"] = "label";

    QJsonObject feature;
    feature["type"] = "Feature";
    feature["geometry"] = geometry;
    feature["properties"] = properties;
    return feature;
}

QByteArray PolygonGeoJsonBuilder::buildFeatureCollection(
    const QVector<MapPolygon>& polygons)
{
    QJsonArray featuresArray;
    for (const auto& poly : polygons) {
        QJsonArray ring = buildRingCoords(poly.coordinates);

        featuresArray.append(buildPolygonFeature(poly, ring));
        featuresArray.append(buildLineStringFeature(poly, ring));

        if (!poly.title.isEmpty()) {
            featuresArray.append(buildPointFeature(poly));
        }
    }

    QJsonObject root;
    root["type"] = "FeatureCollection";
    root["features"] = featuresArray;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}
