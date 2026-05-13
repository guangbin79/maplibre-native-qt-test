#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "geojsonimporter.h"

class tst_GeoJsonImporter : public QObject {
    Q_OBJECT

private slots:
    void testParseAnnotations();
    void testParseRoutes();
    void testParsePolygons();
    void testInvalidJson();
    void testMissingFile();
    void testEmptyFeatureCollection();
};

void tst_GeoJsonImporter::testParseAnnotations()
{
    QByteArray geojson = R"({
        "type": "FeatureCollection",
        "features": [
            {
                "type": "Feature",
                "geometry": {"type": "Point", "coordinates": [116.4074, 39.9042]},
                "properties": {"id": "ann-001", "title": "天安门", "icon": "marker"}
            },
            {
                "type": "Feature",
                "geometry": {"type": "Point", "coordinates": [116.3972, 39.9163]},
                "properties": {"id": "ann-002", "title": "故宫", "icon": "star"}
            }
        ]
    })";

    bool ok = false;
    QVector<MapAnnotation> result = GeoJsonImporter::parseAnnotations(geojson, &ok);

    QVERIFY(ok);
    QCOMPARE(result.size(), 2);

    QCOMPARE(result[0].id, QStringLiteral("ann-001"));
    QCOMPARE(result[0].latitude, 39.9042);
    QCOMPARE(result[0].longitude, 116.4074);
    QCOMPARE(result[0].title, QStringLiteral("天安门"));
    QCOMPARE(result[0].iconName, QStringLiteral("marker"));

    QCOMPARE(result[1].id, QStringLiteral("ann-002"));
    QCOMPARE(result[1].latitude, 39.9163);
    QCOMPARE(result[1].longitude, 116.3972);
    QCOMPARE(result[1].title, QStringLiteral("故宫"));
    QCOMPARE(result[1].iconName, QStringLiteral("star"));
}

void tst_GeoJsonImporter::testParseRoutes()
{
    QByteArray geojson = R"({
        "type": "FeatureCollection",
        "features": [
            {
                "type": "Feature",
                "geometry": {
                    "type": "LineString",
                    "coordinates": [[116.4074, 39.9042], [116.3972, 39.9163], [116.3900, 39.9300]]
                },
                "properties": {
                    "id": "seg-001",
                    "routeId": "route-A",
                    "color": "#ff5722",
                    "width": 4.0,
                    "lineType": "solid",
                    "title": "线路A"
                }
            },
            {
                "type": "Feature",
                "geometry": {
                    "type": "LineString",
                    "coordinates": [[116.3900, 39.9300], [116.3800, 39.9450]]
                },
                "properties": {
                    "id": "seg-002",
                    "routeId": "route-B",
                    "color": "#2196f3",
                    "width": 2.0,
                    "lineType": "dashed",
                    "title": "线路B"
                }
            }
        ]
    })";

    bool ok = false;
    QVector<MapRouteSegment> result = GeoJsonImporter::parseRoutes(geojson, &ok);

    QVERIFY(ok);
    QCOMPARE(result.size(), 2);

    QCOMPARE(result[0].id, QStringLiteral("seg-001"));
    QCOMPARE(result[0].routeId, QStringLiteral("route-A"));
    QCOMPARE(result[0].color, QColor("#ff5722"));
    QCOMPARE(result[0].width, 4.0);
    QCOMPARE(result[0].dashed, false);
    QCOMPARE(result[0].title, QStringLiteral("线路A"));
    QCOMPARE(result[0].coordinates.size(), 3);
    QCOMPARE(result[0].coordinates[0].first, 39.9042);
    QCOMPARE(result[0].coordinates[0].second, 116.4074);
    QCOMPARE(result[0].coordinates[1].first, 39.9163);
    QCOMPARE(result[0].coordinates[1].second, 116.3972);
    QCOMPARE(result[0].coordinates[2].first, 39.9300);
    QCOMPARE(result[0].coordinates[2].second, 116.3900);

    QCOMPARE(result[1].id, QStringLiteral("seg-002"));
    QCOMPARE(result[1].routeId, QStringLiteral("route-B"));
    QCOMPARE(result[1].dashed, true);
    QCOMPARE(result[1].coordinates.size(), 2);
}

void tst_GeoJsonImporter::testParsePolygons()
{
    QByteArray geojson = R"({
        "type": "FeatureCollection",
        "features": [
            {
                "type": "Feature",
                "geometry": {
                    "type": "Polygon",
                    "coordinates": [[[116.4074, 39.9042], [116.3972, 39.9163], [116.3900, 39.9300], [116.4074, 39.9042]]]
                },
                "properties": {
                    "id": "poly-001",
                    "polygonId": "zone-A",
                    "fillEnabled": true,
                    "fillColor": "#ff0000",
                    "fillOpacity": 0.5,
                    "strokeColor": "#000000",
                    "strokeWidth": 2.0,
                    "strokeDashed": false,
                    "title": "区域A"
                }
            }
        ]
    })";

    bool ok = false;
    QVector<MapPolygon> result = GeoJsonImporter::parsePolygons(geojson, &ok);

    QVERIFY(ok);
    QCOMPARE(result.size(), 1);

    QCOMPARE(result[0].id, QStringLiteral("poly-001"));
    QCOMPARE(result[0].polygonId, QStringLiteral("zone-A"));
    QCOMPARE(result[0].fillEnabled, true);
    QCOMPARE(result[0].fillColor, QColor("#ff0000"));
    QCOMPARE(result[0].fillOpacity, 0.5);
    QCOMPARE(result[0].strokeColor, QColor("#000000"));
    QCOMPARE(result[0].strokeWidth, 2.0);
    QCOMPARE(result[0].strokeDashed, false);
    QCOMPARE(result[0].title, QStringLiteral("区域A"));
    QCOMPARE(result[0].coordinates.size(), 4);
    QCOMPARE(result[0].coordinates[0].first, 39.9042);
    QCOMPARE(result[0].coordinates[0].second, 116.4074);
    QCOMPARE(result[0].coordinates[1].first, 39.9163);
    QCOMPARE(result[0].coordinates[1].second, 116.3972);
}

void tst_GeoJsonImporter::testInvalidJson()
{
    bool ok = true;
    QVector<MapAnnotation> annResult = GeoJsonImporter::parseAnnotations("not json", &ok);
    QVERIFY(!ok);
    QVERIFY(annResult.isEmpty());

    ok = true;
    QVector<MapRouteSegment> routeResult = GeoJsonImporter::parseRoutes("not json", &ok);
    QVERIFY(!ok);
    QVERIFY(routeResult.isEmpty());

    ok = true;
    QVector<MapPolygon> polyResult = GeoJsonImporter::parsePolygons("not json", &ok);
    QVERIFY(!ok);
    QVERIFY(polyResult.isEmpty());
}

void tst_GeoJsonImporter::testMissingFile()
{
    bool ok = true;
    QVector<MapAnnotation> annResult = GeoJsonImporter::parseAnnotations(QByteArray(), &ok);
    QVERIFY(!ok);
    QVERIFY(annResult.isEmpty());

    ok = true;
    QVector<MapRouteSegment> routeResult = GeoJsonImporter::parseRoutes(QByteArray(), &ok);
    QVERIFY(!ok);
    QVERIFY(routeResult.isEmpty());

    ok = true;
    QVector<MapPolygon> polyResult = GeoJsonImporter::parsePolygons(QByteArray(), &ok);
    QVERIFY(!ok);
    QVERIFY(polyResult.isEmpty());
}

void tst_GeoJsonImporter::testEmptyFeatureCollection()
{
    QByteArray geojson = R"({"type":"FeatureCollection","features":[]})";

    bool ok = false;
    QVector<MapAnnotation> annResult = GeoJsonImporter::parseAnnotations(geojson, &ok);
    QVERIFY(ok);
    QVERIFY(annResult.isEmpty());

    ok = false;
    QVector<MapRouteSegment> routeResult = GeoJsonImporter::parseRoutes(geojson, &ok);
    QVERIFY(ok);
    QVERIFY(routeResult.isEmpty());

    ok = false;
    QVector<MapPolygon> polyResult = GeoJsonImporter::parsePolygons(geojson, &ok);
    QVERIFY(ok);
    QVERIFY(polyResult.isEmpty());
}

QTEST_MAIN(tst_GeoJsonImporter)
#include "tst_geojsonimporter.moc"
