/**
 * @file tst_geojsonbuilder.cpp
 * @brief GeoJsonBuilder 单元测试
 *
 * 测试 MapAnnotation → GeoJSON FeatureCollection 转换的正确性。
 * 重点关注坐标顺序 [longitude, latitude]（GeoJSON 规范）。
 */

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTest>
#include "geojsonbuilder.h"

class tst_GeoJsonBuilder : public QObject {
    Q_OBJECT

private slots:
    void testEmptyList();
    void testSingleAnnotation();
    void testCoordinateOrder();
    void testMultipleAnnotations();
    void testProperties();
    void testBoundaryCoordinates();
};

void tst_GeoJsonBuilder::testEmptyList()
{
    QVector<MapAnnotation> annotations;
    QByteArray result = GeoJsonBuilder::buildFeatureCollection(annotations);

    QJsonDocument doc = QJsonDocument::fromJson(result);
    QVERIFY(!doc.isNull());
    QCOMPARE(doc.object().value("type").toString(), QString("FeatureCollection"));

    QJsonArray features = doc.object().value("features").toArray();
    QCOMPARE(features.size(), 0);
}

void tst_GeoJsonBuilder::testSingleAnnotation()
{
    MapAnnotation ann;
    ann.id = "poi-001";
    ann.latitude = 39.9042;
    ann.longitude = 116.4074;
    ann.title = "天安门";
    ann.iconName = "marker";

    QByteArray result = GeoJsonBuilder::buildFeatureCollection({ann});

    QJsonDocument doc = QJsonDocument::fromJson(result);
    QVERIFY(!doc.isNull());
    QCOMPARE(doc.object().value("type").toString(), QString("FeatureCollection"));

    QJsonArray features = doc.object().value("features").toArray();
    QCOMPARE(features.size(), 1);

    QJsonObject feature = features[0].toObject();
    QCOMPARE(feature.value("type").toString(), QString("Feature"));
}

void tst_GeoJsonBuilder::testCoordinateOrder()
{
    // GeoJSON 规范：coordinates = [longitude, latitude]
    MapAnnotation ann;
    ann.id = "coord-test";
    ann.latitude = 39.9042;    // 纬度
    ann.longitude = 116.4074;  // 经度

    QByteArray result = GeoJsonBuilder::buildFeatureCollection({ann});

    QJsonDocument doc = QJsonDocument::fromJson(result);
    QJsonObject feature = doc.object().value("features").toArray()[0].toObject();
    QJsonArray coords = feature.value("geometry").toObject().value("coordinates").toArray();

    // coordinates[0] 必须是经度 (longitude)
    QCOMPARE(coords.size(), 2);
    QCOMPARE(coords[0].toDouble(), 116.4074);
    // coordinates[1] 必须是纬度 (latitude)
    QCOMPARE(coords[1].toDouble(), 39.9042);
}

void tst_GeoJsonBuilder::testMultipleAnnotations()
{
    QVector<MapAnnotation> annotations;

    MapAnnotation a1;
    a1.id = "a1"; a1.latitude = 30.0; a1.longitude = 120.0; a1.title = "A1"; a1.iconName = "pin";
    annotations.append(a1);

    MapAnnotation a2;
    a2.id = "a2"; a2.latitude = 31.0; a2.longitude = 121.0; a2.title = "A2"; a2.iconName = "star";
    annotations.append(a2);

    MapAnnotation a3;
    a3.id = "a3"; a3.latitude = 32.0; a3.longitude = 122.0; a3.title = "A3"; a3.iconName = "dot";
    annotations.append(a3);

    QByteArray result = GeoJsonBuilder::buildFeatureCollection(annotations);

    QJsonDocument doc = QJsonDocument::fromJson(result);
    QJsonArray features = doc.object().value("features").toArray();
    QCOMPARE(features.size(), 3);

    // 验证每个 Feature 的 geometry type 都是 Point
    for (int i = 0; i < 3; ++i) {
        QJsonObject geom = features[i].toObject().value("geometry").toObject();
        QCOMPARE(geom.value("type").toString(), QString("Point"));
    }
}

void tst_GeoJsonBuilder::testProperties()
{
    MapAnnotation ann;
    ann.id = "prop-test";
    ann.latitude = 40.0;
    ann.longitude = 100.0;
    ann.title = "测试标题";
    ann.iconName = "flag";

    QByteArray result = GeoJsonBuilder::buildFeatureCollection({ann});

    QJsonDocument doc = QJsonDocument::fromJson(result);
    QJsonObject feature = doc.object().value("features").toArray()[0].toObject();
    QJsonObject props = feature.value("properties").toObject();

    QCOMPARE(props.value("id").toString(), QString("prop-test"));
    QCOMPARE(props.value("title").toString(), QString("测试标题"));
    QCOMPARE(props.value("icon").toString(), QString("flag"));
}

void tst_GeoJsonBuilder::testBoundaryCoordinates()
{
    // 边界值：lat=90, lon=180
    MapAnnotation ann;
    ann.id = "boundary";
    ann.latitude = 90.0;
    ann.longitude = 180.0;

    QByteArray result = GeoJsonBuilder::buildFeatureCollection({ann});

    QJsonDocument doc = QJsonDocument::fromJson(result);
    QJsonObject feature = doc.object().value("features").toArray()[0].toObject();
    QJsonArray coords = feature.value("geometry").toObject().value("coordinates").toArray();

    // coordinates[0] = longitude = 180, coordinates[1] = latitude = 90
    QCOMPARE(coords[0].toDouble(), 180.0);
    QCOMPARE(coords[1].toDouble(), 90.0);
}

QTEST_APPLESS_MAIN(tst_GeoJsonBuilder)
#include "tst_geojsonbuilder.moc"
