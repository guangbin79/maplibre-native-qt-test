#include <QTest>
#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QSignalSpy>
#include <QPainter>
#include <QDir>
#include <QCoreApplication>

#include <QMapLibre/Map>

#include "mainwindow.h"
#include "mapcontainer.h"
#include "testrunner.h"
#include "hxgisserver.h"
#include "mappolygon.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "geojsonexporter.h"
#include "geojsonimporter.h"

class GuiTest : public QObject {
    Q_OBJECT

public:
    explicit GuiTest(QObject *parent = nullptr) : QObject(parent) {}

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testMapLoads();
    void testMapZoom();
    void testMapRotation();
    void testMapPan();
    void testMapTilt();
    void testAnnotationApi();
    void testRouteApi();
    void testLocationApi();
    void testLocationBearingZoomPitch();
    void testAutoRunSequence();
    void testFixedModePanBlocked();
    void testFixedModePanAllowed();
    void testPolygonApi();
    void testPolygonFocus();
    void testGeoJsonExport();
    void testGeoJsonImport();
    void testLayerZOrder();
    void testLocationHeadingUp();
    void testLocationNorthUp();
    void testLocationSimulatedNavigation();

private:
    MainWindow *m_window = nullptr;
    MapContainer *m_map = nullptr;
    TestRunner *m_runner = nullptr;
    HXGISServer *g_server = nullptr;

    void captureScreenshot(const QString &name);
    void log(const QString &msg);

    QMapLibre::Coordinate getMapCenter() const;
};

void GuiTest::initTestCase()
{
    log("initTestCase: starting HXGISServer");
    QString rootPath = "/home/guangbin/Documents/maplibre-native-qt-test/build/linux-x86_64/map_data";
    g_server = new HXGISServer("127.0.0.1:4943", rootPath.toUtf8().constData());
    QVERIFY2(g_server->isRunning(), "Failed to start HXGISServer");
    log(QStringLiteral("HXGISServer started, version: %1").arg(g_server->version()));

    QTest::qWait(500);

    log("initTestCase: creating MainWindow");
    m_window = new MainWindow();
    m_window->resize(800, 600);
    m_window->show();
    QTest::qWait(500);

    m_map = m_window->findChild<MapContainer*>();
    QVERIFY(m_map != nullptr);

    m_runner = m_window->findChild<TestRunner*>();
    QVERIFY(m_runner != nullptr);

    if (!m_map->isMapReady()) {
        QSignalSpy readySpy(m_map, &MapContainer::mapReady);
        QVERIFY(readySpy.wait(20000));
    }
    log("Map ready");

    QTest::qWait(2000);
    captureScreenshot("01_init");
}

void GuiTest::cleanupTestCase()
{
    log("cleanupTestCase: destroying window");
    delete m_window;
    m_window = nullptr;
    delete g_server;
    g_server = nullptr;
}

void GuiTest::captureScreenshot(const QString &name)
{
    QString path = QString("/tmp/test_screenshots/%1.png").arg(name);
    QDir().mkpath("/tmp/test_screenshots");
    QPixmap pixmap = m_window->grab();
    pixmap.save(path);
    log(QStringLiteral("Screenshot saved: %1 (%2x%3)").arg(path).arg(pixmap.width()).arg(pixmap.height()));
}

void GuiTest::log(const QString &msg)
{
    qDebug() << "[GUI_TEST]" << msg;
}

QMapLibre::Coordinate GuiTest::getMapCenter() const
{
    return m_map->map()->coordinate();
}

void GuiTest::testMapLoads()
{
    log("testMapLoads: verifying map widget exists");
    QVERIFY(m_map != nullptr);
    QVERIFY(m_map->map() != nullptr);
    captureScreenshot("02_map_loaded");
}

void GuiTest::testMapZoom()
{
    log("testMapZoom: testing zoom in/out");

    double initialZoom = m_map->map()->zoom();
    log(QStringLiteral("Initial zoom: %1").arg(initialZoom));

    m_map->setZoom(6.0);
    QTest::qWait(1000);
    captureScreenshot("02a_zoom_out");
    double zoomAfterOut = m_map->map()->zoom();
    log(QStringLiteral("After zoom out: %1").arg(zoomAfterOut));
    QVERIFY2(zoomAfterOut <= 6.5, "Zoom out failed");

    m_map->setZoom(12.0);
    QTest::qWait(1000);
    captureScreenshot("02b_zoom_in");
    double zoomAfterIn = m_map->map()->zoom();
    log(QStringLiteral("After zoom in: %1").arg(zoomAfterIn));
    QVERIFY2(zoomAfterIn >= 11.5, "Zoom in failed");

    m_map->setZoom(initialZoom);
    QTest::qWait(500);
    captureScreenshot("02c_zoom_reset");
}

void GuiTest::testMapRotation()
{
    log("testMapRotation: testing bearing rotation");

    double initialBearing = m_map->map()->bearing();
    log(QStringLiteral("Initial bearing: %1").arg(initialBearing));

    m_map->setBearing(45.0);
    QTest::qWait(1000);
    captureScreenshot("02d_rotate_45");
    double bearingAfter45 = m_map->map()->bearing();
    log(QStringLiteral("After 45 deg: %1").arg(bearingAfter45));
    QVERIFY2(qAbs(bearingAfter45 - 45.0) < 5.0, "Rotation to 45 failed");

    m_map->setBearing(90.0);
    QTest::qWait(1000);
    captureScreenshot("02e_rotate_90");
    double bearingAfter90 = m_map->map()->bearing();
    log(QStringLiteral("After 90 deg: %1").arg(bearingAfter90));
    QVERIFY2(qAbs(bearingAfter90 - 90.0) < 5.0, "Rotation to 90 failed");

    m_map->setBearing(180.0);
    QTest::qWait(1000);
    captureScreenshot("02f_rotate_180");
    double bearingAfter180 = m_map->map()->bearing();
    log(QStringLiteral("After 180 deg: %1").arg(bearingAfter180));
    QVERIFY2(qAbs(bearingAfter180 - 180.0) < 5.0 || qAbs(bearingAfter180 + 180.0) < 5.0, "Rotation to 180 failed");

    m_map->setBearing(initialBearing);
    QTest::qWait(500);
    captureScreenshot("02g_rotate_reset");
}

void GuiTest::testMapPan()
{
    log("testMapPan: testing pan to different locations");

    auto initialCoord = m_map->map()->coordinate();
    log(QStringLiteral("Initial center: %1, %2").arg(initialCoord.first).arg(initialCoord.second));

    m_map->setCenter(36.75, 3.05);
    QTest::qWait(1500);
    captureScreenshot("02h_pan_algiers");
    auto coord1 = m_map->map()->coordinate();
    log(QStringLiteral("After pan to Algiers: %1, %2").arg(coord1.first).arg(coord1.second));
    QVERIFY2(qAbs(coord1.first - 36.75) < 0.1 && qAbs(coord1.second - 3.05) < 0.1, "Pan to Algiers failed");

    m_map->setCenter(31.23, 121.47);
    QTest::qWait(1500);
    captureScreenshot("02i_pan_shanghai");
    auto coord2 = m_map->map()->coordinate();
    log(QStringLiteral("After pan to Shanghai: %1, %2").arg(coord2.first).arg(coord2.second));
    QVERIFY2(qAbs(coord2.first - 31.23) < 0.1 && qAbs(coord2.second - 121.47) < 0.1, "Pan to Shanghai failed");

    m_map->setCenter(39.90, 116.41);
    QTest::qWait(1500);
    captureScreenshot("02j_pan_beijing");
    auto coord3 = m_map->map()->coordinate();
    log(QStringLiteral("After pan to Beijing: %1, %2").arg(coord3.first).arg(coord3.second));
    QVERIFY2(qAbs(coord3.first - 39.90) < 0.1 && qAbs(coord3.second - 116.41) < 0.1, "Pan to Beijing failed");

    m_map->setCenter(initialCoord.first, initialCoord.second);
    QTest::qWait(1000);
    captureScreenshot("02k_pan_reset");
}

void GuiTest::testMapTilt()
{
    log("testMapTilt: testing pitch/tilt");

    double initialPitch = m_map->map()->pitch();
    log(QStringLiteral("Initial pitch: %1").arg(initialPitch));

    m_map->setPitch(30.0);
    QTest::qWait(1000);
    captureScreenshot("02l_tilt_30");
    double pitchAfter30 = m_map->map()->pitch();
    log(QStringLiteral("After 30 deg tilt: %1").arg(pitchAfter30));
    QVERIFY2(qAbs(pitchAfter30 - 30.0) < 5.0, "Tilt to 30 failed");

    m_map->setPitch(45.0);
    QTest::qWait(1000);
    captureScreenshot("02m_tilt_45");
    double pitchAfter45 = m_map->map()->pitch();
    log(QStringLiteral("After 45 deg tilt: %1").arg(pitchAfter45));
    QVERIFY2(qAbs(pitchAfter45 - 45.0) < 5.0, "Tilt to 45 failed");

    m_map->setPitch(60.0);
    QTest::qWait(1000);
    captureScreenshot("02n_tilt_60");
    double pitchAfter60 = m_map->map()->pitch();
    log(QStringLiteral("After 60 deg tilt: %1").arg(pitchAfter60));
    QVERIFY2(qAbs(pitchAfter60 - 60.0) < 5.0, "Tilt to 60 failed");

    m_map->setPitch(initialPitch);
    QTest::qWait(500);
    captureScreenshot("02o_tilt_reset");
}

void GuiTest::testAnnotationApi()
{
    log("testAnnotationApi: adding annotations");

    QMap<QString, QImage> icons;
    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::red);
    icons["marker"] = icon;

    QVector<MapAnnotation> anns;
    MapAnnotation a1;
    a1.id = "gui-test-1";
    a1.latitude = 36.75;
    a1.longitude = 3.05;
    a1.title = "Test Point";
    a1.iconName = "marker";
    anns.append(a1);

    m_map->registerAnnotationIcons(icons);
    m_map->setAnnotations(anns);
    QTest::qWait(2000);
    captureScreenshot("03_annotation_added");

    QStringList ids = m_map->allIds();
    QVERIFY(ids.contains("gui-test-1"));
    QCOMPARE(m_map->visibleIds().size(), 1);

    m_map->hideAllAnnotations();
    QTest::qWait(500);
    captureScreenshot("04_annotation_hidden");
    QVERIFY(m_map->visibleIds().isEmpty());

    m_map->showAllAnnotations();
    QTest::qWait(500);
    captureScreenshot("05_annotation_shown");
    QCOMPARE(m_map->visibleIds().size(), 1);

    m_map->clearAnnotations();
    QTest::qWait(500);
    captureScreenshot("06_annotation_cleared");
    QVERIFY(m_map->allIds().isEmpty());
}

void GuiTest::testRouteApi()
{
    log("testRouteApi: adding routes");

    QVector<MapRouteSegment> segs;
    MapRouteSegment seg;
    seg.id = "gui-route-1";
    seg.routeId = "route-A";
    seg.coordinates = {{36.75, 3.05}, {36.76, 3.06}, {36.77, 3.07}};
    seg.color = QColor(255, 0, 0);
    seg.width = 3.0;
    seg.dashed = false;
    segs.append(seg);

    m_map->setRoutes(segs);
    QTest::qWait(2000);
    captureScreenshot("07_route_added");

    QStringList ids = m_map->allRouteIds();
    QVERIFY(ids.contains("route-A"));
    QCOMPARE(m_map->visibleRouteIds().size(), 1);

    m_map->hideAllRoutes();
    QTest::qWait(500);
    captureScreenshot("08_route_hidden");
    QVERIFY(m_map->visibleRouteIds().isEmpty());

    m_map->showAllRoutes();
    QTest::qWait(500);
    captureScreenshot("09_route_shown");
    QCOMPARE(m_map->visibleRouteIds().size(), 1);

    m_map->clearRoutes();
    QTest::qWait(500);
    captureScreenshot("10_route_cleared");
    QVERIFY(m_map->allRouteIds().isEmpty());
}

void GuiTest::testLocationApi()
{
    log("testLocationApi: testing location indicator");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_map->setLocationIcon(icon);
    m_map->setLocation(36.75, 3.05);
    m_map->showLocation();
    QTest::qWait(2000);
    captureScreenshot("11_location_shown");
    QVERIFY(m_map->isLocationVisible());

    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
    m_map->setCenterOffset(200);
    QTest::qWait(1000);
    captureScreenshot("12_location_fixed");
    QCOMPARE(m_map->locationMode(), LocationIndicatorManager::LocationMode::Fixed);

    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Free);
    QTest::qWait(500);
    captureScreenshot("13_location_free");
    QCOMPARE(m_map->locationMode(), LocationIndicatorManager::LocationMode::Free);

    m_map->hideLocation();
    QTest::qWait(500);
    captureScreenshot("14_location_hidden");
    QVERIFY(!m_map->isLocationVisible());
}

void GuiTest::testLocationBearingZoomPitch()
{
    log("testLocationBearingZoomPitch: testing setLocation with bearing/zoom/pitch in Fixed mode");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_map->setLocationIcon(icon);
    m_map->setLocation(36.75, 3.05);
    m_map->showLocation();
    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
    m_map->setCenterOffset(200);
    QTest::qWait(1000);

    double initialBearing = m_map->map()->bearing();
    double initialZoom = m_map->map()->zoom();
    double initialPitch = m_map->map()->pitch();
    log(QStringLiteral("Initial: bearing=%1 zoom=%2 pitch=%3")
        .arg(initialBearing).arg(initialZoom).arg(initialPitch));

    m_map->setLocation(36.75, 3.05, 90.0, 15.0, 45.0);
    QTest::qWait(2000);
    captureScreenshot("15_location_bearing_90_zoom_15_pitch_45");

    double bearingAfter = m_map->map()->bearing();
    double zoomAfter = m_map->map()->zoom();
    double pitchAfter = m_map->map()->pitch();
    log(QStringLiteral("After bearing=90: bearing=%1 zoom=%2 pitch=%3")
        .arg(bearingAfter).arg(zoomAfter).arg(pitchAfter));

    QVERIFY2(qAbs(bearingAfter - 90.0) < 5.0,
             QStringLiteral("Bearing should be ~90, got %1").arg(bearingAfter).toUtf8());
    QVERIFY2(qAbs(zoomAfter - 15.0) < 1.0,
             QStringLiteral("Zoom should be ~15, got %1").arg(zoomAfter).toUtf8());
    QVERIFY2(qAbs(pitchAfter - 45.0) < 5.0,
             QStringLiteral("Pitch should be ~45, got %1").arg(pitchAfter).toUtf8());

    m_map->setLocation(36.75, 3.05, 0.0, 10.0, 0.0);
    QTest::qWait(2000);
    captureScreenshot("15b_location_bearing_0_zoom_10_pitch_0");

    double bearingReset = m_map->map()->bearing();
    double zoomReset = m_map->map()->zoom();
    double pitchReset = m_map->map()->pitch();
    log(QStringLiteral("After reset: bearing=%1 zoom=%2 pitch=%3")
        .arg(bearingReset).arg(zoomReset).arg(pitchReset));

    QVERIFY2(qAbs(bearingReset) < 5.0,
             QStringLiteral("Bearing should be ~0, got %1").arg(bearingReset).toUtf8());
    QVERIFY2(qAbs(zoomReset - 10.0) < 1.0,
             QStringLiteral("Zoom should be ~10, got %1").arg(zoomReset).toUtf8());
    QVERIFY2(qAbs(pitchReset) < 5.0,
             QStringLiteral("Pitch should be ~0, got %1").arg(pitchReset).toUtf8());

    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Free);
    m_map->hideLocation();
}

void GuiTest::testPolygonApi()
{
    log("testPolygonApi: adding polygons");

    QVector<MapPolygon> polys;

    MapPolygon poly1;
    poly1.id = "gui-poly-1";
    poly1.polygonId = "poly-A";
    poly1.coordinates = {{39.92, 116.40}, {39.92, 116.44}, {39.90, 116.44}, {39.90, 116.40}};
    poly1.fillEnabled = true;
    poly1.fillColor = QColor(255, 0, 0);
    poly1.fillOpacity = 0.4;
    poly1.strokeColor = QColor(255, 0, 0);
    poly1.strokeWidth = 2.0;
    poly1.strokeDashed = false;
    poly1.title = QStringLiteral("红色区域");
    polys.append(poly1);

    MapPolygon poly2;
    poly2.id = "gui-poly-2";
    poly2.polygonId = "poly-B";
    poly2.coordinates = {{39.88, 116.38}, {39.88, 116.42}, {39.86, 116.42}, {39.86, 116.38}};
    poly2.fillEnabled = true;
    poly2.fillColor = QColor(0, 0, 255);
    poly2.fillOpacity = 0.3;
    poly2.strokeColor = QColor(0, 0, 255);
    poly2.strokeWidth = 2.0;
    poly2.strokeDashed = true;
    poly2.title = QStringLiteral("蓝色区域");
    polys.append(poly2);

    m_map->setPolygons(polys);
    QTest::qWait(2000);
    captureScreenshot("21_polygon_added");

    QStringList ids = m_map->allPolygonIds();
    QVERIFY(ids.contains("poly-A"));
    QVERIFY(ids.contains("poly-B"));
    QCOMPARE(m_map->visiblePolygonIds().size(), 2);

    m_map->hideAllPolygons();
    QTest::qWait(500);
    captureScreenshot("22_polygon_hidden");
    QVERIFY(m_map->visiblePolygonIds().isEmpty());

    m_map->showAllPolygons();
    QTest::qWait(500);
    captureScreenshot("23_polygon_shown");
    QCOMPARE(m_map->visiblePolygonIds().size(), 2);

    m_map->clearPolygons();
    QTest::qWait(500);
    captureScreenshot("24_polygon_cleared");
    QVERIFY(m_map->allPolygonIds().isEmpty());
}

void GuiTest::testPolygonFocus()
{
    log("testPolygonFocus: testing focusOnPolygon");

    MapPolygon poly;
    poly.id = "gui-poly-focus";
    poly.polygonId = "poly-focus";
    poly.coordinates = {{31.25, 121.45}, {31.25, 121.50}, {31.20, 121.50}, {31.20, 121.45}};
    poly.fillEnabled = true;
    poly.fillColor = QColor(76, 175, 80);
    poly.fillOpacity = 0.5;
    poly.strokeColor = QColor(76, 175, 80);
    poly.strokeWidth = 3.0;
    poly.strokeDashed = false;
    poly.title = QStringLiteral("上海测试");

    m_map->addPolygon(poly);
    QTest::qWait(1000);

    auto centerBefore = m_map->map()->coordinate();
    log(QStringLiteral("Before focus: lat=%1 lon=%2").arg(centerBefore.first).arg(centerBefore.second));

    m_map->focusOnPolygon("poly-focus");
    QTest::qWait(2000);
    captureScreenshot("25_polygon_focus");

    auto centerAfter = m_map->map()->coordinate();
    log(QStringLiteral("After focus: lat=%1 lon=%2").arg(centerAfter.first).arg(centerAfter.second));

    QVERIFY2(qAbs(centerAfter.first - 31.225) < 0.1,
             QStringLiteral("Focus did not move to polygon area (lat), got %1").arg(centerAfter.first).toUtf8());
    QVERIFY2(qAbs(centerAfter.second - 121.475) < 0.1,
             QStringLiteral("Focus did not move to polygon area (lon), got %1").arg(centerAfter.second).toUtf8());

    m_map->clearPolygons();
    QTest::qWait(500);
    captureScreenshot("26_polygon_focus_cleared");
}

void GuiTest::testAutoRunSequence()
{
    log("testAutoRunSequence: running full auto-test sequence");

    QSignalSpy spy(m_runner, &TestRunner::allTestsFinished);
    QVERIFY(spy.isValid());

    m_runner->startTests();

    QVERIFY(spy.wait(60000));
    QCOMPARE(spy.count(), 1);

    QList<QVariant> args = spy.takeFirst();
    int total = args[0].toInt();
    int passed = args[1].toInt();
    int failed = args[2].toInt();

    log(QStringLiteral("Auto-test result: total=%1 passed=%2 failed=%3").arg(total).arg(passed).arg(failed));
    captureScreenshot("15_auto_test_complete");

    QVERIFY2(failed == 0, QStringLiteral("Auto-test had %1 failures").arg(failed).toUtf8());
}

void GuiTest::testFixedModePanBlocked()
{
    log("testFixedModePanBlocked: testing Fixed mode with pan disabled");

    // Setup: show location in Fixed mode, disable touch pan
    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_map->setLocationIcon(icon);
    m_map->setLocation(36.75, 3.05);
    m_map->showLocation();
    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
    m_map->setCenterOffset(400);
    m_map->setFixedTouchPanEnabled(false);
    QTest::qWait(2000);
    captureScreenshot("16_fixed_pan_blocked_setup");

    // Record coordinate before drag
    QMapLibre::Coordinate before = getMapCenter();
    log(QStringLiteral("Before drag: lat=%1 lon=%2").arg(before.first).arg(before.second));

    // Simulate drag on the map widget
    QWidget *mapWidget = m_map->findChild<QWidget*>();
    QVERIFY(mapWidget != nullptr);
    QPoint center = mapWidget->rect().center();

    QTest::mousePress(mapWidget, Qt::LeftButton, {}, center);
    QTest::mouseMove(mapWidget, center + QPoint(200, 0));
    QTest::mouseRelease(mapWidget, Qt::LeftButton, {}, center + QPoint(200, 0));
    QTest::qWait(1000);
    captureScreenshot("17_fixed_pan_blocked_after");

    // Record coordinate after drag
    QMapLibre::Coordinate after = getMapCenter();
    log(QStringLiteral("After drag: lat=%1 lon=%2").arg(after.first).arg(after.second));

    // Verify map did NOT move (drag was blocked)
    double latDiff = qAbs(after.first - before.first);
    double lonDiff = qAbs(after.second - before.second);
    log(QStringLiteral("Coordinate delta: lat=%1 lon=%2").arg(latDiff).arg(lonDiff));

    QVERIFY2(latDiff < 0.0001 && lonDiff < 0.0001,
             QStringLiteral("Map moved when drag should be blocked! delta=(%1, %2)")
                 .arg(latDiff).arg(lonDiff).toUtf8());
}

void GuiTest::testFixedModePanAllowed()
{
    log("testFixedModePanAllowed: testing Fixed mode with pan enabled");

    // Setup: show location in Fixed mode, enable touch pan
    m_map->setFixedTouchPanEnabled(true);
    m_map->locationIndicatorManager()->setFixedTouchResumeTimeout(3000);
    QTest::qWait(1000);
    captureScreenshot("18_fixed_pan_allowed_setup");

    // Record coordinate before drag
    QMapLibre::Coordinate before = getMapCenter();
    log(QStringLiteral("Before drag: lat=%1 lon=%2").arg(before.first).arg(before.second));

    // Simulate drag on the map widget
    QWidget *mapWidget = m_map->findChild<QWidget*>();
    QVERIFY(mapWidget != nullptr);
    QPoint center = mapWidget->rect().center();

    QTest::mousePress(mapWidget, Qt::LeftButton, {}, center);
    QTest::mouseMove(mapWidget, center + QPoint(200, 0));
    QTest::mouseRelease(mapWidget, Qt::LeftButton, {}, center + QPoint(200, 0));
    QTest::qWait(1000);
    captureScreenshot("19_fixed_pan_allowed_after_drag");

    // Record coordinate after drag
    QMapLibre::Coordinate afterDrag = getMapCenter();
    log(QStringLiteral("After drag: lat=%1 lon=%2").arg(afterDrag.first).arg(afterDrag.second));

    // Verify map DID move (drag was allowed)
    double dragLatDiff = qAbs(afterDrag.first - before.first);
    double dragLonDiff = qAbs(afterDrag.second - before.second);
    log(QStringLiteral("Drag delta: lat=%1 lon=%2").arg(dragLatDiff).arg(dragLonDiff));

    QVERIFY2(dragLatDiff > 0.001 || dragLonDiff > 0.001,
             "Map did not move when drag should be allowed!");

    // Wait for auto-resume
    log("Waiting for auto-resume...");
    QTest::qWait(4000);
    captureScreenshot("20_fixed_pan_allowed_after_resume");

    QMapLibre::Coordinate afterResume = getMapCenter();
    log(QStringLiteral("After resume: lat=%1 lon=%2").arg(afterResume.first).arg(afterResume.second));

    // After resume, map should fly back to location (36.75, 3.05)
    double resumeLatDiff = qAbs(afterResume.first - 36.75);
    double resumeLonDiff = qAbs(afterResume.second - 3.05);
    log(QStringLiteral("Resume delta from location: lat=%1 lon=%2").arg(resumeLatDiff).arg(resumeLonDiff));

    QVERIFY2(resumeLatDiff < 0.01 && resumeLonDiff < 0.01,
             QStringLiteral("Map did not resume to location! delta=(%1, %2)")
                 .arg(resumeLatDiff).arg(resumeLonDiff).toUtf8());
}

void GuiTest::testGeoJsonExport() {
    log("testGeoJsonExport: testing GeoJSON export");

    // Step 1: Add test data
    QVector<MapAnnotation> anns;
    MapAnnotation ann;
    ann.id = "gui-export-ann-1";
    ann.latitude = 39.9042;
    ann.longitude = 116.4074;
    ann.title = "ExportTest";
    ann.iconName = "marker";
    anns.append(ann);
    m_map->setAnnotations(anns);

    QVector<MapRouteSegment> segs;
    MapRouteSegment seg;
    seg.id = "gui-export-seg-1";
    seg.routeId = "export-route";
    seg.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}};
    seg.color = QColor("#FF5722");
    seg.width = 4.0;
    seg.dashed = false;
    seg.title = "ExportRoute";
    segs.append(seg);
    m_map->setRoutes(segs);

    QVector<MapPolygon> polys;
    MapPolygon poly;
    poly.id = "gui-export-poly-1";
    poly.polygonId = "export-poly";
    poly.coordinates = {{39.9042, 116.4074}, {39.9163, 116.3972}, {39.9300, 116.3900}};
    poly.fillEnabled = true;
    poly.fillColor = QColor("#FF0000");
    poly.fillOpacity = 0.5;
    poly.strokeColor = QColor("#000000");
    poly.strokeWidth = 2.0;
    poly.strokeDashed = false;
    poly.title = "ExportPoly";
    polys.append(poly);
    m_map->setPolygons(polys);

    QTest::qWait(2000);
    captureScreenshot("27_export_data_ready");

    // Step 2: Export to temp directory
    QString tempDir = QDir::tempPath() + "/geojson_export_test";
    QDir().mkpath(tempDir);

    QByteArray annData = GeoJsonExporter::buildAnnotations(m_map->annotations());
    QFile annFile(tempDir + "/annotations.geojson");
    QVERIFY(annFile.open(QIODevice::WriteOnly));
    annFile.write(annData);
    annFile.close();

    QByteArray routeData = GeoJsonExporter::buildRoutes(m_map->segments());
    QFile routeFile(tempDir + "/routes.geojson");
    QVERIFY(routeFile.open(QIODevice::WriteOnly));
    routeFile.write(routeData);
    routeFile.close();

    QByteArray polyData = GeoJsonExporter::buildPolygons(m_map->polygons());
    QFile polyFile(tempDir + "/polygons.geojson");
    QVERIFY(polyFile.open(QIODevice::WriteOnly));
    polyFile.write(polyData);
    polyFile.close();

    log("Exported to: " + tempDir);

    // Step 3: Verify files exist and are valid JSON
    QVERIFY(QFile::exists(tempDir + "/annotations.geojson"));
    QVERIFY(QFile::exists(tempDir + "/routes.geojson"));
    QVERIFY(QFile::exists(tempDir + "/polygons.geojson"));

    // Verify each file is valid JSON
    QFile f1(tempDir + "/annotations.geojson");
    QVERIFY(f1.open(QIODevice::ReadOnly));
    QJsonDocument d1 = QJsonDocument::fromJson(f1.readAll());
    QVERIFY(!d1.isNull());
    QVERIFY(d1.object()["type"].toString() == "FeatureCollection");
    f1.close();

    captureScreenshot("28_export_completed");

    // Cleanup temp files
    QFile::remove(tempDir + "/annotations.geojson");
    QFile::remove(tempDir + "/routes.geojson");
    QFile::remove(tempDir + "/polygons.geojson");
    QDir().rmdir(tempDir);

    // Clear map
    m_map->clearAnnotations();
    m_map->clearRoutes();
    m_map->clearPolygons();
    QTest::qWait(500);
    captureScreenshot("29_export_cleanup");
}

void GuiTest::testGeoJsonImport() {
    log("testGeoJsonImport: testing GeoJSON import");

    // Step 1: Create test GeoJSON files
    QString tempDir = QDir::tempPath() + "/geojson_import_test";
    QDir().mkpath(tempDir);

    // Create annotations file
    QByteArray annJson = R"({
        "type": "FeatureCollection",
        "features": [{
            "type": "Feature",
            "geometry": {"type": "Point", "coordinates": [116.4074, 39.9042]},
            "properties": {"id": "import-ann-1", "title": "ImportedAnn", "icon": "marker"}
        }]
    })";
    QFile annFile(tempDir + "/annotations.geojson");
    QVERIFY(annFile.open(QIODevice::WriteOnly));
    annFile.write(annJson);
    annFile.close();

    // Create routes file
    QByteArray routeJson = R"({
        "type": "FeatureCollection",
        "features": [{
            "type": "Feature",
            "geometry": {"type": "LineString", "coordinates": [[116.4074,39.9042],[116.3972,39.9163]]},
            "properties": {"id": "import-seg-1", "routeId": "import-route", "color": "#00FF00", "width": 3.0, "lineType": "dashed", "title": "ImportedRoute"}
        }]
    })";
    QFile routeFile(tempDir + "/routes.geojson");
    QVERIFY(routeFile.open(QIODevice::WriteOnly));
    routeFile.write(routeJson);
    routeFile.close();

    // Create polygons file
    QByteArray polyJson = R"({
        "type": "FeatureCollection",
        "features": [{
            "type": "Feature",
            "geometry": {"type": "Polygon", "coordinates": [[[116.4074,39.9042],[116.3972,39.9163],[116.3900,39.9300],[116.4074,39.9042]]]},
            "properties": {"id": "import-poly-1", "polygonId": "import-poly", "fillEnabled": true, "fillColor": "#0000FF", "fillOpacity": 0.7, "strokeColor": "#FFFFFF", "strokeWidth": 4.0, "strokeDashed": true, "title": "ImportedPoly"}
        }]
    })";
    QFile polyFile(tempDir + "/polygons.geojson");
    QVERIFY(polyFile.open(QIODevice::WriteOnly));
    polyFile.write(polyJson);
    polyFile.close();

    // Step 2: Import
    QFile f1(tempDir + "/annotations.geojson");
    QVERIFY(f1.open(QIODevice::ReadOnly));
    bool ok1 = false;
    auto anns = GeoJsonImporter::parseAnnotations(f1.readAll(), &ok1);
    QVERIFY(ok1);
    f1.close();

    QFile f2(tempDir + "/routes.geojson");
    QVERIFY(f2.open(QIODevice::ReadOnly));
    bool ok2 = false;
    auto segs = GeoJsonImporter::parseRoutes(f2.readAll(), &ok2);
    QVERIFY(ok2);
    f2.close();

    QFile f3(tempDir + "/polygons.geojson");
    QVERIFY(f3.open(QIODevice::ReadOnly));
    bool ok3 = false;
    auto polys = GeoJsonImporter::parsePolygons(f3.readAll(), &ok3);
    QVERIFY(ok3);
    f3.close();

    // Step 3: Add to map
    m_map->clearAnnotations();
    m_map->clearRoutes();
    m_map->clearPolygons();

    m_map->setAnnotations(anns);
    m_map->setRoutes(segs);
    m_map->setPolygons(polys);

    QTest::qWait(2000);
    captureScreenshot("30_import_data_loaded");

    // Step 4: Verify
    QVERIFY(m_map->allIds().contains("import-ann-1"));
    QVERIFY(m_map->allRouteIds().contains("import-route"));
    QVERIFY(m_map->allPolygonIds().contains("import-poly"));

    // Verify specific properties
    auto importedAnns = m_map->annotations();
    QCOMPARE(importedAnns.size(), 1);
    QCOMPARE(importedAnns[0].title, QString("ImportedAnn"));

    auto importedSegs = m_map->segments();
    QCOMPARE(importedSegs.size(), 1);
    QCOMPARE(importedSegs[0].color.name(), QString("#00ff00"));
    QVERIFY(importedSegs[0].dashed);

    auto importedPolys = m_map->polygons();
    QCOMPARE(importedPolys.size(), 1);
    QCOMPARE(importedPolys[0].fillColor.name(), QString("#0000ff"));
    QCOMPARE(importedPolys[0].fillOpacity, 0.7);
    QVERIFY(importedPolys[0].strokeDashed);

    captureScreenshot("31_import_verified");

    // Cleanup
    QFile::remove(tempDir + "/annotations.geojson");
    QFile::remove(tempDir + "/routes.geojson");
    QFile::remove(tempDir + "/polygons.geojson");
    QDir().rmdir(tempDir);

    m_map->clearAnnotations();
    m_map->clearRoutes();
    m_map->clearPolygons();
    QTest::qWait(500);
    captureScreenshot("32_import_cleanup");
}

void GuiTest::testLayerZOrder()
{
    log("testLayerZOrder: verifying layer z-order is deterministic");

    // Helper: find index of a layer ID in the layer list
    auto layerIndex = [](const QVector<QString>& layers, const QString& id) -> int {
        for (int i = 0; i < layers.size(); ++i) {
            if (layers[i] == id) return i;
        }
        return -1;
    };

    // Clear all data first
    m_map->clearAnnotations();
    m_map->clearRoutes();
    m_map->clearPolygons();
    QTest::qWait(500);

    // === Scenario 1: Load annotations first, then routes, then polygons ===
    // This is the problematic order that previously caused routes to cover annotations

    // Add annotations first
    QVector<MapAnnotation> anns;
    MapAnnotation ann;
    ann.id = "ztest-ann-1";
    ann.title = "Test Annotation";
    ann.latitude = 39.9142;
    ann.longitude = 116.4074;
    ann.iconName = "default-marker";
    anns.append(ann);
    m_map->setAnnotations(anns);
    QTest::qWait(500);

    // Add routes second (these should NOT cover annotations)
    QVector<MapRouteSegment> segs;
    MapRouteSegment seg;
    seg.id = "ztest-seg-1";
    seg.routeId = "ztest-route";
    seg.coordinates = {{39.9042, 116.3974}, {39.9242, 116.4174}};
    seg.color = QColor(255, 0, 0);
    seg.width = 4.0;
    seg.dashed = false;
    segs.append(seg);
    m_map->setRoutes(segs);
    QTest::qWait(500);

    // Add polygons third
    QVector<MapPolygon> polys;
    MapPolygon poly;
    poly.id = "ztest-poly-1";
    poly.polygonId = "ztest-poly";
    poly.coordinates = {{39.9042, 116.3974}, {39.9242, 116.4174}, {39.9142, 116.4274}, {39.9042, 116.3974}};
    poly.fillEnabled = true;
    poly.fillColor = QColor(0, 0, 255, 100);
    poly.strokeColor = QColor(0, 0, 255);
    poly.strokeWidth = 2.0;
    polys.append(poly);
    m_map->setPolygons(polys);
    QTest::qWait(1000);

    captureScreenshot("33_zorder_annotations_first");

    // Verify layer order: all polygon layers < all route layers < annotations-layer
    QVector<QString> layers = m_map->map()->layerIds();

    int polygonsFillIdx = layerIndex(layers, "polygons-fill");
    int routesSolidIdx = layerIndex(layers, "routes-solid");
    int annotationsIdx = layerIndex(layers, "annotations-layer");

    log(QStringLiteral("Layer order check: polygons-fill=%1, routes-solid=%2, annotations-layer=%3, total=%4")
        .arg(polygonsFillIdx).arg(routesSolidIdx).arg(annotationsIdx).arg(layers.size()));

    QVERIFY2(polygonsFillIdx >= 0, "polygons-fill layer should exist");
    QVERIFY2(routesSolidIdx >= 0, "routes-solid layer should exist");
    QVERIFY2(annotationsIdx >= 0, "annotations-layer should exist");

    QVERIFY2(polygonsFillIdx < routesSolidIdx,
        QStringLiteral("polygons-fill (%1) should be below routes-solid (%2)").arg(polygonsFillIdx).arg(routesSolidIdx).toUtf8());
    QVERIFY2(routesSolidIdx < annotationsIdx,
        QStringLiteral("routes-solid (%1) should be below annotations-layer (%2)").arg(routesSolidIdx).arg(annotationsIdx).toUtf8());

    // === Scenario 2: clearRoutes + setRoutes should preserve order ===
    m_map->clearRoutes();
    QTest::qWait(500);
    m_map->setRoutes(segs);
    QTest::qWait(1000);

    captureScreenshot("34_zorder_after_clear_re_set");

    QVector<QString> layers2 = m_map->map()->layerIds();
    int polygonsFillIdx2 = layerIndex(layers2, "polygons-fill");
    int routesSolidIdx2 = layerIndex(layers2, "routes-solid");
    int annotationsIdx2 = layerIndex(layers2, "annotations-layer");

    log(QStringLiteral("After clear+re-set: polygons-fill=%1, routes-solid=%2, annotations-layer=%3")
        .arg(polygonsFillIdx2).arg(routesSolidIdx2).arg(annotationsIdx2));

    QVERIFY2(polygonsFillIdx2 >= 0, "polygons-fill layer should still exist after route clear+re-set");
    QVERIFY2(routesSolidIdx2 >= 0, "routes-solid layer should exist after re-set");
    QVERIFY2(annotationsIdx2 >= 0, "annotations-layer should still exist after route clear+re-set");

    QVERIFY2(polygonsFillIdx2 < routesSolidIdx2,
        QStringLiteral("After clear+re-set: polygons-fill (%1) should be below routes-solid (%2)").arg(polygonsFillIdx2).arg(routesSolidIdx2).toUtf8());
    QVERIFY2(routesSolidIdx2 < annotationsIdx2,
        QStringLiteral("After clear+re-set: routes-solid (%1) should be below annotations-layer (%2)").arg(routesSolidIdx2).arg(annotationsIdx2).toUtf8());

    log("testLayerZOrder: PASSED - layer z-order is deterministic");
}

void GuiTest::testLocationHeadingUp()
{
    log("testLocationHeadingUp: Fixed mode with HeadingUp");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_map->setLocationIcon(icon);

    // Setup Fixed + HeadingUp
    m_map->setLocation(36.75, 3.05);
    m_map->showLocation();
    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
    m_map->setCenterOffset(200);
    m_map->setFixedTouchPanEnabled(true);
    m_map->locationIndicatorManager()->setFixedTouchResumeTimeout(3000);
    m_map->locationIndicatorManager()->setFixedHeadingMode(
        LocationIndicatorManager::FixedHeadingMode::HeadingUp);
    QTest::qWait(2000);
    captureScreenshot("40_heading_up_initial");

    // Set heading to 90 degrees - map should rotate, icon stays vertical
    m_map->setLocation(36.75, 3.05, 90.0, 15.0, 45.0);
    QTest::qWait(2000);
    captureScreenshot("41_heading_up_90deg");

    double bearing = m_map->map()->bearing();
    log(QStringLiteral("Bearing after heading=90: %1").arg(bearing));
    QVERIFY2(qAbs(bearing - 90.0) < 10.0,
             QStringLiteral("Bearing should be ~90 in HeadingUp, got %1").arg(bearing).toUtf8());

    // Set heading to 180 degrees
    m_map->setLocation(36.75, 3.05, 180.0, 15.0, 45.0);
    QTest::qWait(2000);
    captureScreenshot("42_heading_up_180deg");

    bearing = m_map->map()->bearing();
    log(QStringLiteral("Bearing after heading=180: %1").arg(bearing));
    QVERIFY2(qAbs(bearing - 180.0) < 10.0 || qAbs(bearing + 180.0) < 10.0,
             QStringLiteral("Bearing should be ~180 in HeadingUp, got %1").arg(bearing).toUtf8());

    // Cleanup
    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Free);
    m_map->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testLocationNorthUp()
{
    log("testLocationNorthUp: Fixed mode with NorthUp");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_map->setLocationIcon(icon);

    // Setup Fixed + NorthUp
    m_map->setLocation(36.75, 3.05);
    m_map->showLocation();
    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
    m_map->setCenterOffset(200);
    m_map->setFixedTouchPanEnabled(true);
    m_map->locationIndicatorManager()->setFixedTouchResumeTimeout(3000);
    m_map->locationIndicatorManager()->setFixedHeadingMode(
        LocationIndicatorManager::FixedHeadingMode::NorthUp);
    QTest::qWait(2000);
    captureScreenshot("43_north_up_initial");

    // In NorthUp, map bearing should NOT change when heading changes
    double bearingBefore = m_map->map()->bearing();
    log(QStringLiteral("Bearing before heading change: %1").arg(bearingBefore));

    // Set heading to 90 degrees - map should NOT rotate, icon should rotate
    // IMPORTANT: Use m_map->setLocation() to update follow targets
    m_map->setLocation(36.75, 3.05, 90.0, -1, -1);
    QTest::qWait(2000);
    captureScreenshot("44_north_up_heading_90");

    double bearingAfter = m_map->map()->bearing();
    log(QStringLiteral("Bearing after heading=90 in NorthUp: %1").arg(bearingAfter));
    QVERIFY2(qAbs(bearingAfter - bearingBefore) < 5.0,
             QStringLiteral("Bearing should NOT change in NorthUp mode, before=%1 after=%2")
                 .arg(bearingBefore).arg(bearingAfter).toUtf8());

    // Set heading to 270 degrees
    m_map->setLocation(36.75, 3.05, 270.0, -1, -1);
    QTest::qWait(2000);
    captureScreenshot("45_north_up_heading_270");

    bearingAfter = m_map->map()->bearing();
    log(QStringLiteral("Bearing after heading=270 in NorthUp: %1").arg(bearingAfter));
    QVERIFY2(qAbs(bearingAfter - bearingBefore) < 5.0,
             QStringLiteral("Bearing should still NOT change in NorthUp, before=%1 after=%2")
                 .arg(bearingBefore).arg(bearingAfter).toUtf8());

    // Cleanup
    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Free);
    m_map->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testLocationSimulatedNavigation()
{
    log("testLocationSimulatedNavigation: simulating GPS movement");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_map->setLocationIcon(icon);

    // Setup Fixed + HeadingUp
    m_map->setLocation(36.75, 3.05);
    m_map->showLocation();
    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
    m_map->setCenterOffset(200);
    m_map->setFixedTouchPanEnabled(true);
    m_map->locationIndicatorManager()->setFixedTouchResumeTimeout(3000);
    m_map->locationIndicatorManager()->setFixedHeadingMode(
        LocationIndicatorManager::FixedHeadingMode::HeadingUp);
    QTest::qWait(2000);
    captureScreenshot("46_simnav_start");

    // Simulate 10 GPS updates moving northeast with changing heading
    double lat = 36.75;
    double lon = 3.05;
    double heading = 45.0;

    for (int i = 0; i < 10; ++i) {
        lat += 0.001;
        lon += 0.0008;
        heading += 15.0;
        if (heading >= 360.0) heading -= 360.0;
        // CRITICAL: Use m_map->setLocation() to update follow timer targets
        m_map->setLocation(lat, lon, heading, -1, -1);
        QTest::qWait(500);
    }

    captureScreenshot("47_simnav_after_10_updates");

    // Verify final position
    auto finalCoord = m_map->map()->coordinate();
    log(QStringLiteral("Final position: lat=%1 lon=%2").arg(finalCoord.first).arg(finalCoord.second));
    QVERIFY2(qAbs(finalCoord.first - (36.75 + 0.01)) < 0.01,
             QStringLiteral("Final lat should be near 36.76, got %1").arg(finalCoord.first).toUtf8());

    // Now switch to NorthUp and do more updates
    m_map->locationIndicatorManager()->setFixedHeadingMode(
        LocationIndicatorManager::FixedHeadingMode::NorthUp);
    QTest::qWait(1000);

    for (int i = 0; i < 5; ++i) {
        lat += 0.001;
        lon += 0.0008;
        heading += 30.0;
        if (heading >= 360.0) heading -= 360.0;
        m_map->setLocation(lat, lon, heading, -1, -1);
        QTest::qWait(500);
    }

    captureScreenshot("48_simnav_northup_after_5");

    // Cleanup
    m_map->setLocationMode(LocationIndicatorManager::LocationMode::Free);
    m_map->hideLocation();
    QTest::qWait(500);
}

QTEST_MAIN(GuiTest)
#include "tst_gui.moc"
