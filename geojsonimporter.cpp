#include "geojsonimporter.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>

QVector<MapAnnotation> GeoJsonImporter::parseAnnotations(const QByteArray& geojson, bool* ok)
{
    QJsonDocument doc = QJsonDocument::fromJson(geojson);
    if (!doc.isObject()) { if (ok) *ok = false; return {}; }
    QJsonObject root = doc.object();
    if (root["type"] != "FeatureCollection") { if (ok) *ok = false; return {}; }
    QJsonArray features = root["features"].toArray();

    QVector<MapAnnotation> result;
    for (int i = 0; i < features.size(); i++) {
        QJsonObject feature = features[i].toObject();
        QJsonObject geometry = feature["geometry"].toObject();
        if (geometry.isEmpty()) continue;
        if (geometry["type"].toString() != "Point") continue;

        QJsonObject properties = feature["properties"].toObject();
        QJsonArray coords = geometry["coordinates"].toArray();
        if (coords.size() < 2) continue;

        double lon = coords[0].toDouble();
        double lat = coords[1].toDouble();

        MapAnnotation ann;
        ann.id = properties["id"].toString();
        ann.title = properties["title"].toString();
        ann.iconName = properties["icon"].toString();
        ann.latitude = lat;
        ann.longitude = lon;
        result.append(ann);
    }

    if (ok) *ok = true;
    return result;
}

QVector<MapRouteSegment> GeoJsonImporter::parseRoutes(const QByteArray& geojson, bool* ok)
{
    QJsonDocument doc = QJsonDocument::fromJson(geojson);
    if (!doc.isObject()) { if (ok) *ok = false; return {}; }
    QJsonObject root = doc.object();
    if (root["type"] != "FeatureCollection") { if (ok) *ok = false; return {}; }
    QJsonArray features = root["features"].toArray();

    QVector<MapRouteSegment> result;
    for (int i = 0; i < features.size(); i++) {
        QJsonObject feature = features[i].toObject();
        QJsonObject geometry = feature["geometry"].toObject();
        if (geometry.isEmpty()) continue;
        if (geometry["type"].toString() != "LineString") continue;

        QJsonObject properties = feature["properties"].toObject();
        QJsonArray coordsArray = geometry["coordinates"].toArray();
        if (coordsArray.isEmpty()) continue;

        QVector<QPair<double,double>> coordinates;
        for (int j = 0; j < coordsArray.size(); j++) {
            QJsonArray point = coordsArray[j].toArray();
            if (point.size() < 2) continue;
            double lon = point[0].toDouble();
            double lat = point[1].toDouble();
            coordinates.append({lat, lon});
        }

        MapRouteSegment seg;
        seg.id = properties["id"].toString();
        seg.routeId = properties["routeId"].toString();
        seg.color = QColor(properties["color"].toString());
        seg.width = properties["width"].toDouble();
        seg.dashed = (properties["lineType"].toString() == "dashed");
        seg.title = properties["title"].toString();
        seg.coordinates = coordinates;
        result.append(seg);
    }

    if (ok) *ok = true;
    return result;
}

QVector<MapPolygon> GeoJsonImporter::parsePolygons(const QByteArray& geojson, bool* ok)
{
    QJsonDocument doc = QJsonDocument::fromJson(geojson);
    if (!doc.isObject()) { if (ok) *ok = false; return {}; }
    QJsonObject root = doc.object();
    if (root["type"] != "FeatureCollection") { if (ok) *ok = false; return {}; }
    QJsonArray features = root["features"].toArray();

    QVector<MapPolygon> result;
    for (int i = 0; i < features.size(); i++) {
        QJsonObject feature = features[i].toObject();
        QJsonObject geometry = feature["geometry"].toObject();
        if (geometry.isEmpty()) continue;
        if (geometry["type"].toString() != "Polygon") continue;

        QJsonObject properties = feature["properties"].toObject();
        QJsonArray ringsArray = geometry["coordinates"].toArray();
        if (ringsArray.isEmpty()) continue;
        QJsonArray ring = ringsArray[0].toArray();
        if (ring.isEmpty()) continue;

        QVector<QPair<double,double>> coordinates;
        for (int j = 0; j < ring.size(); j++) {
            QJsonArray point = ring[j].toArray();
            if (point.size() < 2) continue;
            double lon = point[0].toDouble();
            double lat = point[1].toDouble();
            coordinates.append({lat, lon});
        }

        MapPolygon poly;
        poly.id = properties["id"].toString();
        poly.polygonId = properties["polygonId"].toString();
        poly.fillEnabled = properties["fillEnabled"].toBool(true);
        poly.fillColor = QColor(properties["fillColor"].toString());
        poly.fillOpacity = properties["fillOpacity"].toDouble(0.5);
        poly.strokeColor = QColor(properties["strokeColor"].toString());
        poly.strokeWidth = properties["strokeWidth"].toDouble(2.0);
        poly.strokeDashed = properties["strokeDashed"].toBool(false);
        poly.title = properties["title"].toString();
        poly.coordinates = coordinates;
        result.append(poly);
    }

    if (ok) *ok = true;
    return result;
}
