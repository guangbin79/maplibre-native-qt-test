#include <QtTest>
#include <QJsonDocument>
#include "geojsonexporter.h"
#include "geojsonimporter.h"

class tst_GeoJsonRoundTrip : public QObject {
    Q_OBJECT

private slots:
    void testAnnotationRoundTrip();
    void testRouteRoundTrip();
    void testPolygonRoundTrip();
    void testMixedDataRoundTrip();
};

void tst_GeoJsonRoundTrip::testAnnotationRoundTrip()
{
    MapAnnotation original;
    original.id = "test-ann-001";
    original.latitude = 39.9042;
    original.longitude = 116.4074;
    original.title = "天安门";
    original.iconName = "marker";

    QByteArray geojson = GeoJsonExporter::buildAnnotations({original});

    bool ok = false;
    QVector<MapAnnotation> parsed = GeoJsonImporter::parseAnnotations(geojson, &ok);

    QVERIFY(ok);
    QCOMPARE(parsed.size(), 1);
    QCOMPARE(parsed[0].id, original.id);
    QCOMPARE(parsed[0].latitude, original.latitude);
    QCOMPARE(parsed[0].longitude, original.longitude);
    QCOMPARE(parsed[0].title, original.title);
    QCOMPARE(parsed[0].iconName, original.iconName);
}

void tst_GeoJsonRoundTrip::testRouteRoundTrip()
{
    MapRouteSegment original;
    original.id = "test-seg-001";
    original.routeId = "route-A";
    original.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}, {39.9300, 116.3900}};
    original.color = QColor("#FF5722");
    original.width = 4.0;
    original.dashed = false;
    original.title = "线路A";

    QByteArray geojson = GeoJsonExporter::buildRoutes({original});

    bool ok = false;
    QVector<MapRouteSegment> parsed = GeoJsonImporter::parseRoutes(geojson, &ok);

    QVERIFY(ok);
    QCOMPARE(parsed.size(), 1);
    QCOMPARE(parsed[0].id, original.id);
    QCOMPARE(parsed[0].routeId, original.routeId);
    QCOMPARE(parsed[0].color, original.color);
    QCOMPARE(parsed[0].width, original.width);
    QCOMPARE(parsed[0].dashed, original.dashed);
    QCOMPARE(parsed[0].title, original.title);
    QCOMPARE(parsed[0].coordinates.size(), original.coordinates.size());
    for (int i = 0; i < original.coordinates.size(); ++i) {
        QCOMPARE(parsed[0].coordinates[i].first, original.coordinates[i].first);
        QCOMPARE(parsed[0].coordinates[i].second, original.coordinates[i].second);
    }
}

void tst_GeoJsonRoundTrip::testPolygonRoundTrip()
{
    MapPolygon original;
    original.id = "test-poly-001";
    original.polygonId = "zone-A";
    original.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}, {39.9300, 116.3900}};
    original.fillEnabled = true;
    original.fillColor = QColor("#FF0000");
    original.fillOpacity = 0.5;
    original.strokeColor = QColor("#000000");
    original.strokeWidth = 2.0;
    original.strokeDashed = false;
    original.title = "区域A";

    QByteArray geojson = GeoJsonExporter::buildPolygons({original});

    bool ok = false;
    QVector<MapPolygon> parsed = GeoJsonImporter::parsePolygons(geojson, &ok);

    QVERIFY(ok);
    QCOMPARE(parsed.size(), 1);
    QCOMPARE(parsed[0].id, original.id);
    QCOMPARE(parsed[0].polygonId, original.polygonId);
    QCOMPARE(parsed[0].fillEnabled, original.fillEnabled);
    QCOMPARE(parsed[0].fillColor, original.fillColor);
    QCOMPARE(parsed[0].fillOpacity, original.fillOpacity);
    QCOMPARE(parsed[0].strokeColor, original.strokeColor);
    QCOMPARE(parsed[0].strokeWidth, original.strokeWidth);
    QCOMPARE(parsed[0].strokeDashed, original.strokeDashed);
    QCOMPARE(parsed[0].title, original.title);
    // Export auto-closes ring → parsed has one extra point
    QCOMPARE(parsed[0].coordinates.size(), original.coordinates.size() + 1);
    QCOMPARE(parsed[0].coordinates[0].first, original.coordinates[0].first);
    QCOMPARE(parsed[0].coordinates[0].second, original.coordinates[0].second);
    QCOMPARE(parsed[0].coordinates[1].first, original.coordinates[1].first);
    QCOMPARE(parsed[0].coordinates[1].second, original.coordinates[1].second);
    QCOMPARE(parsed[0].coordinates[2].first, original.coordinates[2].first);
    QCOMPARE(parsed[0].coordinates[2].second, original.coordinates[2].second);
}

void tst_GeoJsonRoundTrip::testMixedDataRoundTrip()
{
    MapAnnotation ann;
    ann.id = "ann-mix";
    ann.latitude = 39.9042;
    ann.longitude = 116.4074;
    ann.title = "混合测试";
    ann.iconName = "pin";

    MapRouteSegment seg;
    seg.id = "seg-mix";
    seg.routeId = "route-mix";
    seg.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}};
    seg.color = QColor("#FF5722");
    seg.width = 3.0;
    seg.dashed = true;
    seg.title = "混合线路";

    MapPolygon poly;
    poly.id = "poly-mix";
    poly.polygonId = "zone-mix";
    poly.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}, {39.9300, 116.3900}};
    poly.fillEnabled = true;
    poly.fillColor = QColor("#FF0000");
    poly.fillOpacity = 0.5;
    poly.strokeColor = QColor("#000000");
    poly.strokeWidth = 2.0;
    poly.strokeDashed = false;
    poly.title = "混合区域";

    QByteArray annJson = GeoJsonExporter::buildAnnotations({ann});
    QByteArray segJson = GeoJsonExporter::buildRoutes({seg});
    QByteArray polyJson = GeoJsonExporter::buildPolygons({poly});

    bool annOk = false, segOk = false, polyOk = false;
    QVector<MapAnnotation> parsedAnn = GeoJsonImporter::parseAnnotations(annJson, &annOk);
    QVector<MapRouteSegment> parsedSeg = GeoJsonImporter::parseRoutes(segJson, &segOk);
    QVector<MapPolygon> parsedPoly = GeoJsonImporter::parsePolygons(polyJson, &polyOk);

    QVERIFY(annOk);
    QVERIFY(segOk);
    QVERIFY(polyOk);

    QCOMPARE(parsedAnn.size(), 1);
    QCOMPARE(parsedAnn[0].id, ann.id);
    QCOMPARE(parsedAnn[0].latitude, ann.latitude);
    QCOMPARE(parsedAnn[0].longitude, ann.longitude);
    QCOMPARE(parsedAnn[0].title, ann.title);
    QCOMPARE(parsedAnn[0].iconName, ann.iconName);

    QCOMPARE(parsedSeg.size(), 1);
    QCOMPARE(parsedSeg[0].id, seg.id);
    QCOMPARE(parsedSeg[0].routeId, seg.routeId);
    QCOMPARE(parsedSeg[0].color, seg.color);
    QCOMPARE(parsedSeg[0].width, seg.width);
    QCOMPARE(parsedSeg[0].dashed, seg.dashed);
    QCOMPARE(parsedSeg[0].title, seg.title);

    QCOMPARE(parsedPoly.size(), 1);
    QCOMPARE(parsedPoly[0].id, poly.id);
    QCOMPARE(parsedPoly[0].polygonId, poly.polygonId);
    QCOMPARE(parsedPoly[0].fillEnabled, poly.fillEnabled);
    QCOMPARE(parsedPoly[0].fillColor, poly.fillColor);
    QCOMPARE(parsedPoly[0].fillOpacity, poly.fillOpacity);
    QCOMPARE(parsedPoly[0].strokeColor, poly.strokeColor);
    QCOMPARE(parsedPoly[0].strokeWidth, poly.strokeWidth);
    QCOMPARE(parsedPoly[0].strokeDashed, poly.strokeDashed);
    QCOMPARE(parsedPoly[0].title, poly.title);
}

QTEST_MAIN(tst_GeoJsonRoundTrip)
#include "tst_geojsonroundtrip.moc"
