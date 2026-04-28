#include "geojsonbuilder.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QByteArray GeoJsonBuilder::buildFeatureCollection(const QVector<MapAnnotation>& annotations)
{
    QJsonArray featuresArray;

    for (const auto& ann : annotations) {
        QJsonArray coords;
        // GeoJSON 规范：coordinates = [longitude, latitude]
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
