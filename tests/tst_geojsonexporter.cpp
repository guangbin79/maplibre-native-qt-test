/**
 * @file tst_geojsonexporter.cpp
 * @brief GeoJsonExporter 单元测试
 *
 * 测试 MapAnnotation/MapRouteSegment/MapPolygon → GeoJSON 导出的正确性。
 * 重点关注：
 *   - FeatureCollection 结构
 *   - 坐标顺序 [lon, lat]（GeoJSON 规范）
 *   - 每个 MapPolygon 只生成 1 个 Feature
 *   - 属性完整性
 */

#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "geojsonexporter.h"

class tst_GeoJsonExporter : public QObject {
    Q_OBJECT

private slots:
    void testAnnotations();
    void testRoutes();
    void testPolygons();
    void testEmptyExport();
    void testPolygonAutoClose();
};

void tst_GeoJsonExporter::testAnnotations()
{
    MapAnnotation a1;
    a1.id = "test-ann-001";
    a1.latitude = 39.9042;
    a1.longitude = 116.4074;
    a1.title = "天安门";
    a1.iconName = "marker";

    MapAnnotation a2;
    a2.id = "test-ann-002";
    a2.latitude = 39.9163;
    a2.longitude = 116.3972;
    a2.title = "故宫";
    a2.iconName = "star";

    QByteArray result = GeoJsonExporter::buildAnnotations({a1, a2});

    QJsonDocument doc = QJsonDocument::fromJson(result);
    QVERIFY(doc.isObject());

    QJsonObject root = doc.object();
    QCOMPARE(root["type"].toString(), QStringLiteral("FeatureCollection"));

    QJsonArray features = root["features"].toArray();
    QCOMPARE(features.size(), 2);

    // First annotation
    {
        QJsonObject feature = features[0].toObject();
        QCOMPARE(feature["type"].toString(), QStringLiteral("Feature"));

        QJsonObject geometry = feature["geometry"].toObject();
        QCOMPARE(geometry["type"].toString(), QStringLiteral("Point"));

        QJsonArray coords = geometry["coordinates"].toArray();
        QCOMPARE(coords.size(), 2);
        // GeoJSON: [lon, lat]
        QCOMPARE(coords[0].toDouble(), 116.4074);
        QCOMPARE(coords[1].toDouble(), 39.9042);

        QJsonObject props = feature["properties"].toObject();
        QCOMPARE(props["id"].toString(), QStringLiteral("test-ann-001"));
        QCOMPARE(props["title"].toString(), QStringLiteral("天安门"));
        QCOMPARE(props["icon"].toString(), QStringLiteral("marker"));
    }

    // Second annotation
    {
        QJsonObject feature = features[1].toObject();
        QCOMPARE(feature["type"].toString(), QStringLiteral("Feature"));

        QJsonObject geometry = feature["geometry"].toObject();
        QCOMPARE(geometry["type"].toString(), QStringLiteral("Point"));

        QJsonArray coords = geometry["coordinates"].toArray();
        QCOMPARE(coords.size(), 2);
        QCOMPARE(coords[0].toDouble(), 116.3972);
        QCOMPARE(coords[1].toDouble(), 39.9163);

        QJsonObject props = feature["properties"].toObject();
        QCOMPARE(props["id"].toString(), QStringLiteral("test-ann-002"));
        QCOMPARE(props["title"].toString(), QStringLiteral("故宫"));
        QCOMPARE(props["icon"].toString(), QStringLiteral("star"));
    }
}

void tst_GeoJsonExporter::testRoutes()
{
    MapRouteSegment seg1;
    seg1.id = "test-seg-001";
    seg1.routeId = "route-A";
    seg1.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}, {39.9300, 116.3900}};
    seg1.color = QColor("#FF5722");
    seg1.width = 4.0;
    seg1.dashed = false;
    seg1.title = "线路A";

    MapRouteSegment seg2;
    seg2.id = "test-seg-002";
    seg2.routeId = "route-B";
    seg2.coordinates = {{39.9300, 116.3900}, {39.9450, 116.3800}};
    seg2.color = QColor("#2196F3");
    seg2.width = 2.0;
    seg2.dashed = true;
    seg2.title = "线路B";

    QByteArray result = GeoJsonExporter::buildRoutes({seg1, seg2});

    QJsonDocument doc = QJsonDocument::fromJson(result);
    QVERIFY(doc.isObject());

    QJsonObject root = doc.object();
    QCOMPARE(root["type"].toString(), QStringLiteral("FeatureCollection"));

    QJsonArray features = root["features"].toArray();
    QCOMPARE(features.size(), 2);

    // First route: solid
    {
        QJsonObject feature = features[0].toObject();
        QCOMPARE(feature["type"].toString(), QStringLiteral("Feature"));

        QJsonObject geometry = feature["geometry"].toObject();
        QCOMPARE(geometry["type"].toString(), QStringLiteral("LineString"));

        QJsonArray coords = geometry["coordinates"].toArray();
        QCOMPARE(coords.size(), 3);

        // First point: [lon, lat]
        QCOMPARE(coords[0].toArray()[0].toDouble(), 116.4074);
        QCOMPARE(coords[0].toArray()[1].toDouble(), 39.9042);
        // Second point
        QCOMPARE(coords[1].toArray()[0].toDouble(), 116.3972);
        QCOMPARE(coords[1].toArray()[1].toDouble(), 39.9163);
        // Third point
        QCOMPARE(coords[2].toArray()[0].toDouble(), 116.3900);
        QCOMPARE(coords[2].toArray()[1].toDouble(), 39.9300);

        QJsonObject props = feature["properties"].toObject();
        QCOMPARE(props["id"].toString(), QStringLiteral("test-seg-001"));
        QCOMPARE(props["routeId"].toString(), QStringLiteral("route-A"));
        QCOMPARE(props["color"].toString(), QStringLiteral("#ff5722"));
        QCOMPARE(props["width"].toDouble(), 4.0);
        QCOMPARE(props["lineType"].toString(), QStringLiteral("solid"));
        QCOMPARE(props["title"].toString(), QStringLiteral("线路A"));
    }

    // Second route: dashed
    {
        QJsonObject feature = features[1].toObject();
        QJsonObject props = feature["properties"].toObject();
        QCOMPARE(props["id"].toString(), QStringLiteral("test-seg-002"));
        QCOMPARE(props["routeId"].toString(), QStringLiteral("route-B"));
        QCOMPARE(props["lineType"].toString(), QStringLiteral("dashed"));
        QCOMPARE(props["color"].toString(), QStringLiteral("#2196f3"));
        QCOMPARE(props["width"].toDouble(), 2.0);
        QCOMPARE(props["title"].toString(), QStringLiteral("线路B"));
    }
}

void tst_GeoJsonExporter::testPolygons()
{
    MapPolygon poly1;
    poly1.id = "test-poly-001";
    poly1.polygonId = "zone-A";
    poly1.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}, {39.9300, 116.3900}};
    poly1.fillEnabled = true;
    poly1.fillColor = QColor("#FF0000");
    poly1.fillOpacity = 0.5;
    poly1.strokeColor = QColor("#000000");
    poly1.strokeWidth = 2.0;
    poly1.strokeDashed = false;
    poly1.title = "区域A";

    MapPolygon poly2;
    poly2.id = "test-poly-002";
    poly2.polygonId = "zone-B";
    poly2.coordinates = {{39.91, 116.40}, {39.92, 116.41}, {39.93, 116.42}};
    poly2.fillEnabled = false;
    poly2.fillColor = QColor("#00FF00");
    poly2.fillOpacity = 0.3;
    poly2.strokeColor = QColor("#333333");
    poly2.strokeWidth = 1.5;
    poly2.strokeDashed = true;
    poly2.title = "区域B";

    QByteArray result = GeoJsonExporter::buildPolygons({poly1, poly2});

    QJsonDocument doc = QJsonDocument::fromJson(result);
    QVERIFY(doc.isObject());

    QJsonObject root = doc.object();
    QCOMPARE(root["type"].toString(), QStringLiteral("FeatureCollection"));

    QJsonArray features = root["features"].toArray();
    // Each polygon produces exactly 1 Feature (NOT 3)
    QCOMPARE(features.size(), 2);

    // First polygon
    {
        QJsonObject feature = features[0].toObject();
        QCOMPARE(feature["type"].toString(), QStringLiteral("Feature"));

        QJsonObject geometry = feature["geometry"].toObject();
        QCOMPARE(geometry["type"].toString(), QStringLiteral("Polygon"));

        // Polygon coordinates: [[[lon,lat],...]]
        QJsonArray rings = geometry["coordinates"].toArray();
        QCOMPARE(rings.size(), 1);
        QJsonArray ring = rings[0].toArray();
        // 3 original + 1 auto-closed = 4 points
        QCOMPARE(ring.size(), 4);

        // First point: [lon, lat]
        QCOMPARE(ring[0].toArray()[0].toDouble(), 116.4074);
        QCOMPARE(ring[0].toArray()[1].toDouble(), 39.9042);

        QJsonObject props = feature["properties"].toObject();
        QCOMPARE(props["id"].toString(), QStringLiteral("test-poly-001"));
        QCOMPARE(props["polygonId"].toString(), QStringLiteral("zone-A"));
        QCOMPARE(props["fillEnabled"].toBool(), true);
        QCOMPARE(props["fillColor"].toString(), QStringLiteral("#ff0000"));
        QCOMPARE(props["fillOpacity"].toDouble(), 0.5);
        QCOMPARE(props["strokeColor"].toString(), QStringLiteral("#000000"));
        QCOMPARE(props["strokeWidth"].toDouble(), 2.0);
        QCOMPARE(props["strokeDashed"].toBool(), false);
        QCOMPARE(props["title"].toString(), QStringLiteral("区域A"));
    }

    // Second polygon
    {
        QJsonObject feature = features[1].toObject();
        QJsonObject props = feature["properties"].toObject();
        QCOMPARE(props["id"].toString(), QStringLiteral("test-poly-002"));
        QCOMPARE(props["polygonId"].toString(), QStringLiteral("zone-B"));
        QCOMPARE(props["fillEnabled"].toBool(), false);
        QCOMPARE(props["strokeDashed"].toBool(), true);
        QCOMPARE(props["title"].toString(), QStringLiteral("区域B"));
    }
}

void tst_GeoJsonExporter::testEmptyExport()
{
    QByteArray annResult = GeoJsonExporter::buildAnnotations({});
    QJsonDocument annDoc = QJsonDocument::fromJson(annResult);
    QVERIFY(annDoc.isObject());
    QCOMPARE(annDoc.object()["type"].toString(), QStringLiteral("FeatureCollection"));
    QCOMPARE(annDoc.object()["features"].toArray().size(), 0);

    QByteArray routeResult = GeoJsonExporter::buildRoutes({});
    QJsonDocument routeDoc = QJsonDocument::fromJson(routeResult);
    QVERIFY(routeDoc.isObject());
    QCOMPARE(routeDoc.object()["type"].toString(), QStringLiteral("FeatureCollection"));
    QCOMPARE(routeDoc.object()["features"].toArray().size(), 0);

    QByteArray polyResult = GeoJsonExporter::buildPolygons({});
    QJsonDocument polyDoc = QJsonDocument::fromJson(polyResult);
    QVERIFY(polyDoc.isObject());
    QCOMPARE(polyDoc.object()["type"].toString(), QStringLiteral("FeatureCollection"));
    QCOMPARE(polyDoc.object()["features"].toArray().size(), 0);
}

void tst_GeoJsonExporter::testPolygonAutoClose()
{
    MapPolygon poly;
    poly.id = "tri-auto";
    poly.polygonId = "tri-zone";
    // Unclosed triangle: 3 points, first != last
    poly.coordinates = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 1.0}};
    poly.fillColor = QColor("#FF0000");
    poly.strokeColor = QColor("#000000");
    poly.strokeWidth = 2.0;

    QByteArray result = GeoJsonExporter::buildPolygons({poly});

    QJsonDocument doc = QJsonDocument::fromJson(result);
    QJsonArray features = doc.object()["features"].toArray();
    QCOMPARE(features.size(), 1);

    QJsonArray ring = features[0].toObject()["geometry"].toObject()["coordinates"].toArray()[0].toArray();
    // 3 original + 1 auto-closed = 4
    QCOMPARE(ring.size(), 4);

    // Last point must equal first point
    QCOMPARE(ring[0].toArray()[0].toDouble(), ring[3].toArray()[0].toDouble());
    QCOMPARE(ring[0].toArray()[1].toDouble(), ring[3].toArray()[1].toDouble());
}

QTEST_MAIN(tst_GeoJsonExporter)
#include "tst_geojsonexporter.moc"
