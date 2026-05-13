#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "polygongeojsonbuilder.h"

class PolygonGeoJsonBuilderTest : public QObject {
    Q_OBJECT

private slots:
    void testEmptyInput()
    {
        QByteArray result = PolygonGeoJsonBuilder::buildFeatureCollection({});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QVERIFY(doc.isObject());
        QCOMPARE(doc.object()["type"].toString(), QStringLiteral("FeatureCollection"));
        QCOMPARE(doc.object()["features"].toArray().size(), 0);
    }

    void testSinglePolygonNoTitle()
    {
        MapPolygon poly;
        poly.id = "p1";
        poly.polygonId = "zone-A";
        poly.coordinates = {{39.9, 116.3}, {39.92, 116.35}, {39.95, 116.4}};
        poly.fillColor = QColor("#FF0000");
        poly.strokeColor = QColor("#000000");
        poly.strokeWidth = 2.0;
        poly.title = "";

        QByteArray result = PolygonGeoJsonBuilder::buildFeatureCollection({poly});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonArray features = doc.object()["features"].toArray();
        QCOMPARE(features.size(), 2);

        QCOMPARE(features[0].toObject()["geometry"].toObject()["type"].toString(), QStringLiteral("Polygon"));
        QCOMPARE(features[1].toObject()["geometry"].toObject()["type"].toString(), QStringLiteral("LineString"));
    }

    void testSinglePolygonWithTitle()
    {
        MapPolygon poly;
        poly.id = "p1";
        poly.polygonId = "zone-A";
        poly.coordinates = {{39.9, 116.3}, {39.92, 116.35}, {39.95, 116.4}};
        poly.fillColor = QColor("#FF0000");
        poly.strokeColor = QColor("#000000");
        poly.strokeWidth = 2.0;
        poly.title = "Area A";

        QByteArray result = PolygonGeoJsonBuilder::buildFeatureCollection({poly});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonArray features = doc.object()["features"].toArray();
        QCOMPARE(features.size(), 3);

        QCOMPARE(features[0].toObject()["geometry"].toObject()["type"].toString(), QStringLiteral("Polygon"));
        QCOMPARE(features[1].toObject()["geometry"].toObject()["type"].toString(), QStringLiteral("LineString"));
        QCOMPARE(features[2].toObject()["geometry"].toObject()["type"].toString(), QStringLiteral("Point"));
    }

    void testRingAutoClosure()
    {
        MapPolygon poly;
        poly.id = "tri";
        poly.polygonId = "tri-zone";
        poly.coordinates = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 1.0}};
        poly.fillColor = QColor("#FF0000");
        poly.strokeColor = QColor("#000000");
        poly.strokeWidth = 2.0;

        QByteArray result = PolygonGeoJsonBuilder::buildFeatureCollection({poly});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonArray features = doc.object()["features"].toArray();

        QJsonArray polyRing = features[0].toObject()["geometry"].toObject()["coordinates"].toArray()[0].toArray();
        QCOMPARE(polyRing.size(), 4);
        QCOMPARE(polyRing[0].toArray()[0].toDouble(), polyRing[3].toArray()[0].toDouble());
        QCOMPARE(polyRing[0].toArray()[1].toDouble(), polyRing[3].toArray()[1].toDouble());

        QJsonArray lineCoords = features[1].toObject()["geometry"].toObject()["coordinates"].toArray();
        QCOMPARE(lineCoords.size(), 4);
        QCOMPARE(lineCoords[0].toArray()[0].toDouble(), lineCoords[3].toArray()[0].toDouble());
        QCOMPARE(lineCoords[0].toArray()[1].toDouble(), lineCoords[3].toArray()[1].toDouble());
    }

    void testCoordinateSwap()
    {
        MapPolygon poly;
        poly.id = "swap";
        poly.polygonId = "swap-zone";
        poly.coordinates = {{39.9, 116.3}};
        poly.fillColor = QColor("#000000");
        poly.strokeColor = QColor("#000000");
        poly.strokeWidth = 1.0;

        QByteArray result = PolygonGeoJsonBuilder::buildFeatureCollection({poly});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonArray polyRing = doc.object()["features"][0].toObject()["geometry"].toObject()["coordinates"].toArray()[0].toArray();
        QJsonArray firstPt = polyRing[0].toArray();
        QVERIFY(qAbs(firstPt[0].toDouble() - 116.3) < 0.001);
        QVERIFY(qAbs(firstPt[1].toDouble() - 39.9) < 0.001);
    }

    void testCentroidComputation()
    {
        MapPolygon poly;
        poly.id = "rect";
        poly.polygonId = "rect-zone";
        poly.coordinates = {{0.0, 0.0}, {0.0, 2.0}, {2.0, 2.0}, {2.0, 0.0}};
        poly.fillColor = QColor("#FF0000");
        poly.strokeColor = QColor("#000000");
        poly.strokeWidth = 1.0;
        poly.title = "Rectangle";

        QByteArray result = PolygonGeoJsonBuilder::buildFeatureCollection({poly});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonArray features = doc.object()["features"].toArray();

        QJsonObject pointFeature;
        for (const auto& f : features) {
            if (f.toObject()["geometry"].toObject()["type"].toString() == "Point") {
                pointFeature = f.toObject();
                break;
            }
        }
        QVERIFY(!pointFeature.isEmpty());

        QJsonArray centroid = pointFeature["geometry"].toObject()["coordinates"].toArray();
        QVERIFY(qAbs(centroid[0].toDouble() - 1.0) < 0.001);
        QVERIFY(qAbs(centroid[1].toDouble() - 1.0) < 0.001);
    }

    void testFillOpacity()
    {
        MapPolygon poly;
        poly.id = "opacity";
        poly.polygonId = "op-zone";
        poly.coordinates = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 1.0}};
        poly.fillColor = QColor("#FF0000");
        poly.fillOpacity = 0.75;
        poly.strokeColor = QColor("#000000");
        poly.strokeWidth = 2.0;

        QByteArray result = PolygonGeoJsonBuilder::buildFeatureCollection({poly});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonObject props = doc.object()["features"][0].toObject()["properties"].toObject();
        QCOMPARE(props["fillOpacity"].toDouble(), 0.75);
    }

    void testStrokeTypeDashed()
    {
        MapPolygon poly;
        poly.id = "dashed";
        poly.polygonId = "dash-zone";
        poly.coordinates = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 1.0}};
        poly.fillColor = QColor("#FF0000");
        poly.strokeColor = QColor("#000000");
        poly.strokeWidth = 2.0;
        poly.strokeDashed = true;

        QByteArray result = PolygonGeoJsonBuilder::buildFeatureCollection({poly});
        QJsonDocument doc = QJsonDocument::fromJson(result);

        QJsonObject polyProps = doc.object()["features"][0].toObject()["properties"].toObject();
        QCOMPARE(polyProps["strokeType"].toString(), QStringLiteral("dashed"));

        QJsonObject lineProps = doc.object()["features"][1].toObject()["properties"].toObject();
        QCOMPARE(lineProps["strokeType"].toString(), QStringLiteral("dashed"));
    }

    void testStrokeTypeSolid()
    {
        MapPolygon poly;
        poly.id = "solid";
        poly.polygonId = "solid-zone";
        poly.coordinates = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 1.0}};
        poly.fillColor = QColor("#FF0000");
        poly.strokeColor = QColor("#000000");
        poly.strokeWidth = 2.0;
        poly.strokeDashed = false;

        QByteArray result = PolygonGeoJsonBuilder::buildFeatureCollection({poly});
        QJsonDocument doc = QJsonDocument::fromJson(result);

        QJsonObject polyProps = doc.object()["features"][0].toObject()["properties"].toObject();
        QCOMPARE(polyProps["strokeType"].toString(), QStringLiteral("solid"));

        QJsonObject lineProps = doc.object()["features"][1].toObject()["properties"].toObject();
        QCOMPARE(lineProps["strokeType"].toString(), QStringLiteral("solid"));
    }
};

QTEST_MAIN(PolygonGeoJsonBuilderTest)
#include "tst_polygongeojsonbuilder.moc"
