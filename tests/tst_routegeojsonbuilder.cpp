#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "routegeojsonbuilder.h"

class RouteGeoJsonBuilderTest : public QObject {
    Q_OBJECT

private slots:
    void testEmptyList()
    {
        QByteArray result = RouteGeoJsonBuilder::buildFeatureCollection({});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QVERIFY(doc.isObject());
        QCOMPARE(doc.object()["type"].toString(), QStringLiteral("FeatureCollection"));
        QCOMPARE(doc.object()["features"].toArray().size(), 0);
    }

    void testSingleSegment()
    {
        MapRouteSegment seg;
        seg.id = "s1";
        seg.routeId = "route-A";
        seg.coordinates = {{39.9, 116.3}, {39.92, 116.35}};
        seg.color = QColor("#FF0000");
        seg.width = 4.0;
        seg.dashed = false;
        seg.title = "Route A";

        QByteArray result = RouteGeoJsonBuilder::buildFeatureCollection({seg});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonArray features = doc.object()["features"].toArray();
        QCOMPARE(features.size(), 1);

        QJsonObject feature = features[0].toObject();
        QCOMPARE(feature["type"].toString(), QStringLiteral("Feature"));

        QJsonObject geometry = feature["geometry"].toObject();
        QCOMPARE(geometry["type"].toString(), QStringLiteral("LineString"));

        QJsonArray coords = geometry["coordinates"].toArray();
        QCOMPARE(coords.size(), 2);

        // First point: [lon, lat] = [116.3, 39.9]
        QCOMPARE(coords[0].toArray()[0].toDouble(), 116.3);
        QCOMPARE(coords[0].toArray()[1].toDouble(), 39.9);

        // Second point: [lon, lat] = [116.35, 39.92]
        QCOMPARE(coords[1].toArray()[0].toDouble(), 116.35);
        QCOMPARE(coords[1].toArray()[1].toDouble(), 39.92);
    }

    void testCoordinateOrder()
    {
        MapRouteSegment seg;
        seg.id = "coord-test";
        seg.color = QColor("#000000");
        seg.width = 1.0;
        seg.coordinates = {{39.9, 116.3}, {40.0, 117.0}};

        QByteArray result = RouteGeoJsonBuilder::buildFeatureCollection({seg});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonArray coords = doc.object()["features"][0].toObject()["geometry"].toObject()["coordinates"].toArray();

        QVERIFY(qAbs(coords[0].toArray()[0].toDouble() - 116.3) < 0.001);
        QVERIFY(qAbs(coords[0].toArray()[1].toDouble() - 39.9) < 0.001);
    }

    void testMultipleSegments()
    {
        MapRouteSegment seg1;
        seg1.id = "s1";
        seg1.routeId = "route-A";
        seg1.color = QColor("#FF0000");
        seg1.width = 3.0;
        seg1.coordinates = {{39.9, 116.3}, {39.92, 116.35}};

        MapRouteSegment seg2;
        seg2.id = "s2";
        seg2.routeId = "route-A";
        seg2.color = QColor("#0000FF");
        seg2.width = 2.0;
        seg2.dashed = true;
        seg2.coordinates = {{39.92, 116.35}, {39.95, 116.4}};

        QByteArray result = RouteGeoJsonBuilder::buildFeatureCollection({seg1, seg2});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonArray features = doc.object()["features"].toArray();
        QCOMPARE(features.size(), 2);
    }

    void testProperties()
    {
        MapRouteSegment seg;
        seg.id = "prop-test";
        seg.routeId = "route-B";
        seg.color = QColor("#00FF00");
        seg.width = 5.0;
        seg.dashed = true;
        seg.title = "Green Route";
        seg.coordinates = {{0.0, 0.0}, {1.0, 1.0}};

        QByteArray result = RouteGeoJsonBuilder::buildFeatureCollection({seg});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonObject props = doc.object()["features"][0].toObject()["properties"].toObject();

        QCOMPARE(props["id"].toString(), QStringLiteral("prop-test"));
        QCOMPARE(props["routeId"].toString(), QStringLiteral("route-B"));
        QCOMPARE(props["lineType"].toString(), QStringLiteral("dashed"));
        QCOMPARE(props["color"].toString(), QStringLiteral("#00ff00"));
        QCOMPARE(props["width"].toDouble(), 5.0);
        QCOMPARE(props["title"].toString(), QStringLiteral("Green Route"));
    }

    void testSolidLineType()
    {
        MapRouteSegment seg;
        seg.id = "solid-test";
        seg.color = QColor("#000000");
        seg.width = 1.0;
        seg.dashed = false;
        seg.coordinates = {{0.0, 0.0}, {1.0, 1.0}};

        QByteArray result = RouteGeoJsonBuilder::buildFeatureCollection({seg});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonObject props = doc.object()["features"][0].toObject()["properties"].toObject();
        QCOMPARE(props["lineType"].toString(), QStringLiteral("solid"));
    }

    void testManyCoordinates()
    {
        MapRouteSegment seg;
        seg.id = "many-points";
        seg.color = QColor("#000000");
        seg.width = 2.0;
        for (int i = 0; i < 100; ++i) {
            seg.coordinates.append({0.0 + i * 0.01, 0.0 + i * 0.01});
        }

        QByteArray result = RouteGeoJsonBuilder::buildFeatureCollection({seg});
        QJsonDocument doc = QJsonDocument::fromJson(result);
        QJsonArray coords = doc.object()["features"][0].toObject()["geometry"].toObject()["coordinates"].toArray();
        QCOMPARE(coords.size(), 100);
    }
};

QTEST_MAIN(RouteGeoJsonBuilderTest)
#include "tst_routegeojsonbuilder.moc"
