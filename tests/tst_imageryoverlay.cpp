#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QDir>
#include <QFileInfo>

#include "imageryoverlaymanager.h"
#include "mapcontainer.h"
#include "hxgisserver.h"
#include <QMapLibre/Map>

class TestImageryOverlay : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialState();
    void testTilesUrl();
    void testConstants();
    void testVisibilityToggles();
    void testNullMapCallOrderSafe();
    void testSatelliteLayerRegistered();
    void testVisibilityToggleOnMap();
    void testAdversarialLayerOrder();
    void cleanupTestCase();

private:
    MapContainer *m_map = nullptr;
    HXGISServer *m_server = nullptr;
};

void TestImageryOverlay::initTestCase()
{
    // Test binary lives in <BD>/tests/, data root in <BD>/map_data
    QString rootPath = QDir(QCoreApplication::applicationDirPath() + "/../map_data").path();
    if (!QFileInfo::exists(rootPath + "/mvt/styles/day/style.json")) {
        QSKIP("map_data basemap missing");
    }

    m_server = new HXGISServer("127.0.0.1:4943", rootPath.toUtf8().constData());
    QVERIFY2(m_server->isRunning(), "HXGISServer failed to start (port 4943 busy?)");
    QTest::qWait(500);

    MapContainer::MapConfig config;
    config.styleUrl = "http://127.0.0.1:4943/mvt/styles/day/style.json?schema=hxmap";
    config.defaultCoordinate = QMapLibre::Coordinate(33.57, -7.62); // Casablanca, within imagery z11-14
    config.defaultZoom = 12.0;

    m_map = new MapContainer(config);
    m_map->resize(800, 600);
    m_map->show();

    QCoreApplication::processEvents();
    QTest::qWait(500);

    if (!m_map->isMapReady()) {
        QSignalSpy readySpy(m_map, &MapContainer::mapReady);
        if (!readySpy.wait(8000)) {
            // Rare SDK startup stall (~1 in 3 runs): style load finishes at the
            // network layer but DidFinishLoadingMap is never delivered. Kicking a
            // style reload recovers it. mapReady must still fire for real.
            if (!m_map->isMapReady()) m_map->setStyle(config.styleUrl);
            readySpy.wait(12000);
        }
    }
    QVERIFY2(m_map->isMapReady(), "mapReady never fired within 20s");
}

void TestImageryOverlay::testInitialState()
{
    ImageryOverlayManager mgr(nullptr);
    QVERIFY(mgr.isVisible());
}

void TestImageryOverlay::testTilesUrl()
{
    QCOMPARE(ImageryOverlayManager::tilesUrl(),
             QString("http://127.0.0.1:4943/gisserver/mbtiles/{z}/{x}/{y}/imagery"));
}

void TestImageryOverlay::testConstants()
{
    QCOMPARE(QString(ImageryOverlayManager::SOURCE_ID), QString("imagery"));
    QCOMPARE(QString(ImageryOverlayManager::LAYER_ID), QString("satellite"));
}

void TestImageryOverlay::testVisibilityToggles()
{
    ImageryOverlayManager mgr(nullptr);
    mgr.setMapReady(true);
    QVERIFY(mgr.isVisible());
    mgr.setVisible(false);
    QVERIFY(!mgr.isVisible());
    mgr.setVisible(true);
    QVERIFY(mgr.isVisible());
}

void TestImageryOverlay::testNullMapCallOrderSafe()
{
    ImageryOverlayManager mgr(nullptr);
    mgr.setVisible(false);
    QVERIFY(!mgr.isVisible());
    mgr.setMapReady(true);
    QVERIFY(!mgr.isVisible());
    mgr.setVisible(true);
    QVERIFY(mgr.isVisible());
    mgr.setMapReady(false);
    QVERIFY(mgr.isVisible());
}

// Full-map case (ii): satellite layer registered by the wired manager at mapReady
void TestImageryOverlay::testSatelliteLayerRegistered()
{
    QVERIFY(m_map->map()->layerExists("satellite"));
    QVERIFY(m_map->map()->layerIds().contains("satellite"));
    // Let tile fetches settle: with imagery data absent, fetches fail as
    // 204-with-body and layer/process must survive (tolerance proof).
    QTest::qWait(1500);
    QVERIFY(m_map->map()->layerExists("satellite"));
}

// Full-map case (iv): visibility toggle keeps manager state and layer in sync
void TestImageryOverlay::testVisibilityToggleOnMap()
{
    ImageryOverlayManager *mgr = m_map->imageryOverlayManager();
    QVERIFY(mgr != nullptr);
    QVERIFY(mgr->isVisible());

    mgr->setVisible(false);
    QVERIFY(!mgr->isVisible());
    QVERIFY(m_map->map()->layerExists("satellite"));

    mgr->setVisible(true);
    QVERIFY(mgr->isVisible());
    QVERIFY(m_map->map()->layerExists("satellite"));
}

// Full-map case (v), adversarial z-order: rebuild the app stack so the only
// existing app layers are [location-indicator-layer, annotations-layer]
// (bottom->top); a fresh manager must still anchor satellite BELOW both.
// Fails under candidate-priority anchor degradation AND under top-down
// layerIds semantics.
void TestImageryOverlay::testAdversarialLayerOrder()
{
    m_map->map()->removeLayer("satellite");
    m_map->map()->removeSource("imagery");
    m_map->map()->removeLayer("annotations-layer");
    m_map->map()->addLayer("location-indicator-layer", QVariantMap{
        {"type", "background"},
        {"paint", QVariantMap{{"background-color", "#000000"}}},
    });
    m_map->map()->addLayer("annotations-layer", QVariantMap{
        {"type", "background"},
        {"paint", QVariantMap{{"background-color", "#000000"}}},
    });

    ImageryOverlayManager fresh(m_map->map());
    fresh.setMapReady(true);

    const QVector<QString> ids = m_map->map()->layerIds();
    const int satellite = ids.indexOf("satellite");
    QVERIFY2(satellite >= 0, "satellite layer was not registered");
    QVERIFY2(satellite < ids.indexOf("location-indicator-layer"),
             qPrintable(QString("satellite (%1) must be below location-indicator-layer (%2); layerIds: %3")
                            .arg(satellite)
                            .arg(ids.indexOf("location-indicator-layer"))
                            .arg(QStringList(ids).join(", "))));
    QVERIFY2(satellite < ids.indexOf("annotations-layer"),
             qPrintable(QString("satellite (%1) must be below annotations-layer (%2); layerIds: %3")
                            .arg(satellite)
                            .arg(ids.indexOf("annotations-layer"))
                            .arg(QStringList(ids).join(", "))));
}

void TestImageryOverlay::cleanupTestCase()
{
    delete m_map;
    m_map = nullptr;
    delete m_server;
    m_server = nullptr;
}

QTEST_MAIN(TestImageryOverlay)
#include "tst_imageryoverlay.moc"
