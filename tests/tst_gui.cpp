#include <QTest>
#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QSignalSpy>
#include <QPainter>
#include <QDir>
#include <QCoreApplication>
#include <QTouchEvent>
#include <QPointingDevice>
#include <QInputDevice>
#include <QtGui/private/qeventpoint_p.h>
#include <QElapsedTimer>
#include <QMap>
#include <memory>

#include <QMapLibre/Map>

#include "mainwindow.h"
#include "mapcontainer.h"
#include "testrunner.h"
#include "locationindicatormanager.h"
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
    void testLocationNorthUpSmoothRotation();
    void testLocationSimulatedNavigation();
    void testTouchInfra();
    void testTouchPinchZoomRegression();
    void testTouchDoubleTapZoomInZoomToCursor();
    void testTouchTwoFingerTapZoomOutTriggers();
    void testTouchDoubleTapDoesNotRecenter();

    // Regression tests: setZoom/setPitch in FixedFollowing browse-mode bug
    void testSetZoomDoesNotTriggerBrowse();
    void testSetPitchDoesNotTriggerBrowse();
    void testUserPanDetectedTriggersBrowse();
    void testUserZoomDetectedTriggersBrowse();
    void testExternalMapChangeTriggersBrowse();
    void testResumeAfterBrowse();
    void testFollowAnimationStillWorks();
    void testSetZoomThenDrag();
    void testFollowBearingChangedHeadingUp();
    void testFollowBearingChangedNorthUpDrift();
    void testFollowBearingChangedOnModeSwitch();
    void testMapContainerBearingBridgedAndLastBearingSynced();

private:
    MainWindow *m_window = nullptr;
    MapContainer *m_map = nullptr;
    LocationIndicatorManager* m_locationIndicatorManager = nullptr;
    TestRunner *m_runner = nullptr;
    HXGISServer *g_server = nullptr;

    QPointingDevice* m_touchDevice = nullptr;
    QElapsedTimer m_touchTimer;

    void captureScreenshot(const QString &name);
    void log(const QString &msg);

    std::unique_ptr<QTouchEvent> buildTouchEvent(QEvent::Type type, const QList<QPair<QEventPoint::State, QPointF>>& statePosPairs, qint64 timestampMs);
    void sendTouch(MapContainer* m, const QTouchEvent& e);

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

    m_touchDevice = QTest::createTouchDevice(QInputDevice::DeviceType::TouchScreen);
    QVERIFY(m_touchDevice != nullptr);
    m_touchTimer.start();

    m_map = m_window->findChild<MapContainer*>();
    QVERIFY(m_map != nullptr);

    // Create Manager for test and wire signals
    m_locationIndicatorManager = new LocationIndicatorManager(m_map);
    connect(m_map, &MapContainer::userPanDetected,
            m_locationIndicatorManager, &LocationIndicatorManager::pauseFollowing);
    connect(m_map, &MapContainer::userZoomDetected,
            m_locationIndicatorManager, &LocationIndicatorManager::pauseFollowing);

    m_runner = m_window->findChild<TestRunner*>();
    QVERIFY(m_runner != nullptr);

    if (!m_map->isMapReady()) {
        QSignalSpy readySpy(m_map, &MapContainer::mapReady);
        QVERIFY(readySpy.wait(20000));
    }
    m_locationIndicatorManager->initMap(m_map->map());
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
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    QTest::qWait(2000);
    captureScreenshot("11_location_shown");
    QVERIFY(m_locationIndicatorManager->isLocationVisible());

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    QTest::qWait(1000);
    captureScreenshot("12_location_fixed");
    QCOMPARE(m_locationIndicatorManager->mode(), LocationIndicatorManager::LocationMode::Fixed);

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    QTest::qWait(500);
    captureScreenshot("13_location_free");
    QCOMPARE(m_locationIndicatorManager->mode(), LocationIndicatorManager::LocationMode::Free);

    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
    captureScreenshot("14_location_hidden");
    QVERIFY(!m_locationIndicatorManager->isLocationVisible());
}

void GuiTest::testLocationBearingZoomPitch()
{
    log("testLocationBearingZoomPitch: testing setLocation with bearing/zoom/pitch in Fixed mode");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    QTest::qWait(1000);

    double initialBearing = m_map->map()->bearing();
    double initialZoom = m_map->map()->zoom();
    double initialPitch = m_map->map()->pitch();
    log(QStringLiteral("Initial: bearing=%1 zoom=%2 pitch=%3")
        .arg(initialBearing).arg(initialZoom).arg(initialPitch));

    m_locationIndicatorManager->setLocation({36.75, 3.05, 90.0});
    m_locationIndicatorManager->setZoom(15.0);
    m_locationIndicatorManager->setPitch(45.0);
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

    m_locationIndicatorManager->setLocation({36.75, 3.05, 0.0});
    m_locationIndicatorManager->setZoom(10.0);
    m_locationIndicatorManager->setPitch(0.0);
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

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
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
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(400);
    m_map->setUserInteractionEnabled(false);
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
    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
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
    m_locationIndicatorManager->setLocationIcon(icon);

    // Setup Fixed + HeadingUp
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
    m_locationIndicatorManager->setFixedHeadingMode(
        LocationIndicatorManager::FixedHeadingMode::HeadingUp);
    QTest::qWait(2000);
    captureScreenshot("40_heading_up_initial");

    // Set heading to 90 degrees - map should rotate, icon stays vertical
    m_locationIndicatorManager->setLocation({36.75, 3.05, 90.0});
    m_locationIndicatorManager->setZoom(15.0);
    m_locationIndicatorManager->setPitch(45.0);
    QTest::qWait(2000);
    captureScreenshot("41_heading_up_90deg");

    double bearing = m_map->map()->bearing();
    log(QStringLiteral("Bearing after heading=90: %1").arg(bearing));
    QVERIFY2(qAbs(bearing - 90.0) < 10.0,
             QStringLiteral("Bearing should be ~90 in HeadingUp, got %1").arg(bearing).toUtf8());

    // Set heading to 180 degrees
    m_locationIndicatorManager->setLocation({36.75, 3.05, 180.0});
    m_locationIndicatorManager->setZoom(15.0);
    m_locationIndicatorManager->setPitch(45.0);
    QTest::qWait(2000);
    captureScreenshot("42_heading_up_180deg");

    bearing = m_map->map()->bearing();
    log(QStringLiteral("Bearing after heading=180: %1").arg(bearing));
    QVERIFY2(qAbs(bearing - 180.0) < 10.0 || qAbs(bearing + 180.0) < 10.0,
             QStringLiteral("Bearing should be ~180 in HeadingUp, got %1").arg(bearing).toUtf8());

    // Cleanup
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testLocationNorthUp()
{
    log("testLocationNorthUp: Fixed mode with NorthUp");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);

    // Setup Fixed + NorthUp
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
    m_locationIndicatorManager->setFixedHeadingMode(
        LocationIndicatorManager::FixedHeadingMode::NorthUp);
    QTest::qWait(2000);
    captureScreenshot("43_north_up_initial");

    // In NorthUp, map bearing should NOT change when heading changes
    double bearingBefore = m_map->map()->bearing();
    log(QStringLiteral("Bearing before heading change: %1").arg(bearingBefore));

    // Set heading to 90 degrees - map should NOT rotate, icon should rotate
    // IMPORTANT: Use setLocation() to update follow targets
    m_locationIndicatorManager->setLocation({36.75, 3.05, 90.0});
    QTest::qWait(2000);
    captureScreenshot("44_north_up_heading_90");

    double bearingAfter = m_map->map()->bearing();
    log(QStringLiteral("Bearing after heading=90 in NorthUp: %1").arg(bearingAfter));
    QVERIFY2(qAbs(bearingAfter - bearingBefore) < 5.0,
             QStringLiteral("Bearing should NOT change in NorthUp mode, before=%1 after=%2")
                 .arg(bearingBefore).arg(bearingAfter).toUtf8());

    // Set heading to 270 degrees
    m_locationIndicatorManager->setLocation({36.75, 3.05, 270.0});
    QTest::qWait(2000);
    captureScreenshot("45_north_up_heading_270");

    bearingAfter = m_map->map()->bearing();
    log(QStringLiteral("Bearing after heading=270 in NorthUp: %1").arg(bearingAfter));
    QVERIFY2(qAbs(bearingAfter - bearingBefore) < 5.0,
             QStringLiteral("Bearing should still NOT change in NorthUp, before=%1 after=%2")
                 .arg(bearingBefore).arg(bearingAfter).toUtf8());

    // Cleanup
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testLocationNorthUpSmoothRotation()
{
    // Setup: create overlay (icon + showLocation), Fixed mode + NorthUp, settle at heading 90.
    // Mirrors testLocationNorthUp setup — showLocation() is REQUIRED so m_overlay exists;
    // otherwise the setLocation animation enqueue (guarded by m_overlay) never fires.
    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05, 90.0});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(
        LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
    m_locationIndicatorManager->setFixedHeadingMode(
        LocationIndicatorManager::FixedHeadingMode::NorthUp);
    QTest::qWait(500);  // wait for snap to settle at heading 90

    // AC8: Verify animation is non-instantaneous (50ms should not complete 300ms animation)
    // Re-establish FixedFollowing state: the NorthUp forced setBearing(0) during the
    // settle can trip pan-detection (-> FixedBrowsing); showLocation() is the canonical
    // API to (re-)enter following mode, which is the documented precondition for the
    // overlay-rotation animation enqueue in setLocation().
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setLocation({36.75, 3.05, 180.0});  // 90 → 180
    QTest::qWait(50);
    const double angleMid = m_locationIndicatorManager->currentOverlayAngle();
    QVERIFY2(qAbs(angleMid - 180.0) > 5.0,
             "Animation should not complete within 50ms (300ms duration)");

    // Wait for animation to complete
    QTest::qWait(500);
    const double angleAfter = m_locationIndicatorManager->currentOverlayAngle();
    QVERIFY2(qAbs(angleAfter - 180.0) < 1.0,
             "Animation should converge to target within 500ms");

    // AC9: Anti-jitter — rapid small heading changes should produce smooth output
    // Input stream: [170, 185, 175, 190, 180] — max input delta = |190-175| = 15
    const double jitterAngles[] = {170.0, 185.0, 175.0, 190.0, 180.0};
    QVector<double> observed;
    observed.append(angleAfter);
    for (double h : jitterAngles) {
        m_locationIndicatorManager->setLocation({36.75, 3.05, h});
        QTest::qWait(50);
        observed.append(m_locationIndicatorManager->currentOverlayAngle());
    }
    double maxObservedDelta = 0.0;
    for (int i = 1; i < observed.size(); ++i) {
        maxObservedDelta = std::max(maxObservedDelta,
                                     std::abs(observed[i] - observed[i-1]));
    }
    QVERIFY2(maxObservedDelta < 15.0,
             "Animation should smooth jitter (low-pass filter)");
}

void GuiTest::testLocationSimulatedNavigation()
{
    log("testLocationSimulatedNavigation: simulating GPS movement");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);

    // Setup Fixed + HeadingUp
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
    m_locationIndicatorManager->setFixedHeadingMode(
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
        // CRITICAL: Use setLocation() to update follow timer targets
        m_locationIndicatorManager->setLocation({lat, lon, heading});
        QTest::qWait(500);
    }

    captureScreenshot("47_simnav_after_10_updates");

    // Verify final position
    auto finalCoord = m_map->map()->coordinate();
    log(QStringLiteral("Final position: lat=%1 lon=%2").arg(finalCoord.first).arg(finalCoord.second));
    QVERIFY2(qAbs(finalCoord.first - (36.75 + 0.01)) < 0.01,
             QStringLiteral("Final lat should be near 36.76, got %1").arg(finalCoord.first).toUtf8());

    // Now switch to NorthUp and do more updates
    m_locationIndicatorManager->setFixedHeadingMode(
        LocationIndicatorManager::FixedHeadingMode::NorthUp);
    QTest::qWait(1000);

    for (int i = 0; i < 5; ++i) {
        lat += 0.001;
        lon += 0.0008;
        heading += 30.0;
        if (heading >= 360.0) heading -= 360.0;
        m_locationIndicatorManager->setLocation({lat, lon, heading});
        QTest::qWait(500);
    }

    captureScreenshot("48_simnav_northup_after_5");

    // Cleanup
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

std::unique_ptr<QTouchEvent> GuiTest::buildTouchEvent(QEvent::Type type, const QList<QPair<QEventPoint::State, QPointF>>& statePosPairs, qint64 timestampMs)
{
    QList<QEventPoint> points;
    for (int i = 0; i < statePosPairs.size(); ++i) {
        const auto& pair = statePosPairs[i];
        QEventPoint::State state = pair.first;
        QPointF pos = pair.second;

        QEventPoint pt(i, state, pos, m_map->mapToGlobal(pos));
        QMutableEventPoint::setTimestamp(pt, timestampMs);
        if (state == QEventPoint::State::Pressed) {
            QMutableEventPoint::setPressTimestamp(pt, timestampMs);
        }
        points.append(pt);
    }

    return std::make_unique<QTouchEvent>(type, m_touchDevice, Qt::NoModifier, points);
}

void GuiTest::sendTouch(MapContainer* m, const QTouchEvent& e)
{
    QCoreApplication::sendEvent(m, const_cast<QTouchEvent*>(&e));
}

void GuiTest::testTouchInfra()
{
    log("testTouchInfra: sending single TouchBegin event");

    if (QGuiApplication::platformName() == "wayland")
        QSKIP("QTBUG-107157");

    QVERIFY(QTest::qWaitForWindowExposed(m_window->windowHandle()));
    QVERIFY2(m_map->testAttribute(Qt::WA_AcceptTouchEvents), "MapContainer must accept touch");

    QPointF center = m_map->rect().center();
    qint64 ts = m_touchTimer.elapsed();

    QList<QPair<QEventPoint::State, QPointF>> statePosPairs;
    statePosPairs.append({QEventPoint::State::Pressed, center});

    auto ev = buildTouchEvent(QEvent::TouchBegin, statePosPairs, ts);
    sendTouch(m_map, *ev);

    QTest::qWait(500);
    log("testTouchInfra: PASSED - no crash");
}

void GuiTest::testTouchPinchZoomRegression()
{
    log("testTouchPinchZoomRegression: two-finger spread → zoom in");

    if (QGuiApplication::platformName() == "wayland")
        QSKIP("QTBUG-107157");

    QVERIFY(QTest::qWaitForWindowExposed(m_window->windowHandle()));
    QVERIFY2(m_map->testAttribute(Qt::WA_AcceptTouchEvents), "MapContainer must accept touch");

    m_map->setZoom(8.0);
    for (int retry = 0; retry < 30 && qAbs(m_map->map()->zoom() - 8.0) > 0.01; ++retry) {
        QTest::qWait(500);
        m_map->setZoom(8.0);
    }
    double zoomBefore = m_map->map()->zoom();
    QCOMPARE(zoomBefore, 8.0);

    const qreal w = m_map->width();
    const qreal h = m_map->height();
    const QPointF f1Start(w * 0.45, h * 0.5);
    const QPointF f2Start(w * 0.55, h * 0.5);
    const QPointF f1End(w * 0.35, h * 0.5);
    const QPointF f2End(w * 0.65, h * 0.5);

    qint64 t0 = m_touchTimer.elapsed();

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Pressed, f1Start});
        auto ev = buildTouchEvent(QEvent::TouchBegin, pts, t0);
        sendTouch(m_map, *ev);
    }

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Stationary, f1Start});
        pts.append({QEventPoint::State::Pressed, f2Start});
        auto ev = buildTouchEvent(QEvent::TouchUpdate, pts, t0 + 20);
        sendTouch(m_map, *ev);
    }

    auto lerp = [](const QPointF& a, const QPointF& b, qreal t) -> QPointF {
        return QPointF(a.x() + (b.x() - a.x()) * t, a.y() + (b.y() - a.y()) * t);
    };

    for (int step = 1; step <= 8; ++step) {
        qreal frac = step / 8.0;
        QPointF p1 = lerp(f1Start, f1End, frac);
        QPointF p2 = lerp(f2Start, f2End, frac);
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Updated, p1});
        pts.append({QEventPoint::State::Updated, p2});
        auto ev = buildTouchEvent(QEvent::TouchUpdate, pts, t0 + step * 30);
        sendTouch(m_map, *ev);
        QTest::qWait(10);
    }

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Released, f1End});
        pts.append({QEventPoint::State::Released, f2End});
        auto ev = buildTouchEvent(QEvent::TouchEnd, pts, t0 + 150);
        sendTouch(m_map, *ev);
    }

    QTest::qWait(500);

    double zoomAfter = m_map->map()->zoom();
    QVERIFY2(zoomAfter > 8.0,
              qPrintable(QString("Expected zoom > 8.0 after pinch spread, got %1").arg(zoomAfter)));

    log(qPrintable(QString("testTouchPinchZoomRegression: PASSED — zoom %1 → %2")
                   .arg(zoomBefore).arg(zoomAfter)));
}

void GuiTest::testTouchDoubleTapZoomInZoomToCursor()
{
    log("testTouchDoubleTapZoomInZoomToCursor: double-tap off-center → zoom, anchor held");

    if (QGuiApplication::platformName() == "wayland")
        QSKIP("QTBUG-107157");

    QVERIFY(QTest::qWaitForWindowExposed(m_window->windowHandle()));
    QVERIFY2(m_map->testAttribute(Qt::WA_AcceptTouchEvents), "MapContainer must accept touch");

    m_map->setZoom(8.0);
    for (int retry = 0; retry < 30 && qAbs(m_map->map()->zoom() - 8.0) > 0.01; ++retry) {
        QTest::qWait(500);
        m_map->setZoom(8.0);
    }
    QCOMPARE(m_map->map()->zoom(), 8.0);

    const qreal w = m_map->width();
    const qreal h = m_map->height();
    const QPointF TAP(w * 0.3, h * 0.3);

    QMapLibre::Coordinate tapCoordBefore = m_map->screenToCoordinate(TAP);

    qint64 t0 = m_touchTimer.elapsed();

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Pressed, TAP});
        auto evDown = buildTouchEvent(QEvent::TouchBegin, pts, t0);
        sendTouch(m_map, *evDown);

        pts[0].first = QEventPoint::State::Released;
        auto evUp = buildTouchEvent(QEvent::TouchEnd, pts, t0 + 30);
        sendTouch(m_map, *evUp);
    }

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Pressed, TAP});
        auto evDown = buildTouchEvent(QEvent::TouchBegin, pts, t0 + 180);
        sendTouch(m_map, *evDown);

        pts[0].first = QEventPoint::State::Released;
        auto evUp = buildTouchEvent(QEvent::TouchEnd, pts, t0 + 210);
        sendTouch(m_map, *evUp);
    }

    QTest::qWait(500);

    double zoomAfter = m_map->map()->zoom();
    QVERIFY2(zoomAfter > 8.5 && zoomAfter < 10.0,
             qPrintable(QString("Expected zoom ~9.0 after double-tap, got %1").arg(zoomAfter)));

    QMapLibre::Coordinate tapCoordAfter = m_map->screenToCoordinate(TAP);
    qreal latDrift = std::abs(tapCoordAfter.first - tapCoordBefore.first);
    qreal lonDrift = std::abs(tapCoordAfter.second - tapCoordBefore.second);

    QVERIFY2(latDrift < 0.3 && lonDrift < 0.3,
             qPrintable(QString("Anchor drift: lat=%1° lon=%2° — double-tap recentered the map!")
                        .arg(latDrift, 0, 'f', 8).arg(lonDrift, 0, 'f', 8)));

    log("testTouchDoubleTapZoomInZoomToCursor: PASSED — zoom-to-cursor math holds anchor within 0.3°");
}

void GuiTest::testTouchTwoFingerTapZoomOutTriggers()
{
    log("testTouchTwoFingerTapZoomOutTriggers: two-finger tap → zoom out");

    if (QGuiApplication::platformName() == "wayland")
        QSKIP("QTBUG-107157");

    QVERIFY(QTest::qWaitForWindowExposed(m_window->windowHandle()));
    QVERIFY2(m_map->testAttribute(Qt::WA_AcceptTouchEvents), "MapContainer must accept touch");

    m_map->setZoom(8.0);
    for (int retry = 0; retry < 30 && qAbs(m_map->map()->zoom() - 8.0) > 0.01; ++retry) {
        QTest::qWait(500);
        m_map->setZoom(8.0);
    }
    QCOMPARE(m_map->map()->zoom(), 8.0);

    const qreal w = m_map->width();
    const qreal h = m_map->height();
    const QPointF F1(w * 0.4, h * 0.5);
    const QPointF F2(w * 0.6, h * 0.5);

    qint64 t0 = m_touchTimer.elapsed();

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Pressed, F1});
        auto ev = buildTouchEvent(QEvent::TouchBegin, pts, t0);
        sendTouch(m_map, *ev);
    }

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Stationary, F1});
        pts.append({QEventPoint::State::Pressed, F2});
        auto ev = buildTouchEvent(QEvent::TouchUpdate, pts, t0 + 20);
        sendTouch(m_map, *ev);
    }

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Stationary, F1});
        pts.append({QEventPoint::State::Stationary, F2});
        auto ev = buildTouchEvent(QEvent::TouchUpdate, pts, t0 + 60);
        sendTouch(m_map, *ev);
    }

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Released, F1});
        pts.append({QEventPoint::State::Released, F2});
        auto ev = buildTouchEvent(QEvent::TouchEnd, pts, t0 + 90);
        sendTouch(m_map, *ev);
    }

    QTest::qWait(1000);

    double zoomAfter = m_map->map()->zoom();
    QVERIFY2(zoomAfter < 7.6,
             qPrintable(QString("Expected zoom < 7.5 after two-finger-tap, got %1").arg(zoomAfter)));

    log("testTouchTwoFingerTapZoomOutTriggers: PASSED — two-finger-tap zoom-out fired");
}

void GuiTest::testTouchDoubleTapDoesNotRecenter()
{
    log("testTouchDoubleTapDoesNotRecenter: double-tap off-center should not move map center");

    if (QGuiApplication::platformName() == "wayland")
        QSKIP("QTBUG-107157");

    QVERIFY(QTest::qWaitForWindowExposed(m_window->windowHandle()));
    QVERIFY2(m_map->testAttribute(Qt::WA_AcceptTouchEvents), "MapContainer must accept touch");

    m_map->setZoom(8.0);
    for (int retry = 0; retry < 30; ++retry) {
        if (std::abs(m_map->map()->zoom() - 8.0) < 0.01) break;
        QTest::qWait(500);
    }
    QCOMPARE(m_map->map()->zoom(), 8.0);

    const qreal w = m_map->width();
    const qreal h = m_map->height();
    const QPointF TAP(w * 0.3, h * 0.3);

    QMapLibre::Coordinate centerBefore = getMapCenter();
    QMapLibre::Coordinate tapCoord = m_map->screenToCoordinate(TAP);

    qreal initialSeparation = std::abs(centerBefore.first - tapCoord.first)
                             + std::abs(centerBefore.second - tapCoord.second);
    QVERIFY2(initialSeparation > 0.01,
             "Test precondition failed: off-center tap point differs from map center");

    qint64 t0 = m_touchTimer.elapsed();

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Pressed, TAP});
        auto evDown = buildTouchEvent(QEvent::TouchBegin, pts, t0);
        sendTouch(m_map, *evDown);

        pts[0].first = QEventPoint::State::Released;
        auto evUp = buildTouchEvent(QEvent::TouchEnd, pts, t0 + 30);
        sendTouch(m_map, *evUp);
    }

    {
        QList<QPair<QEventPoint::State, QPointF>> pts;
        pts.append({QEventPoint::State::Pressed, TAP});
        auto evDown = buildTouchEvent(QEvent::TouchBegin, pts, t0 + 180);
        sendTouch(m_map, *evDown);

        pts[0].first = QEventPoint::State::Released;
        auto evUp = buildTouchEvent(QEvent::TouchEnd, pts, t0 + 210);
        sendTouch(m_map, *evUp);
    }

    QTest::qWait(500);

    double zoomAfter = m_map->map()->zoom();
    QVERIFY2(zoomAfter > 8.5,
             qPrintable(QString("Double-tap did not fire, test invalid — zoom still %1").arg(zoomAfter)));

    QMapLibre::Coordinate centerAfter = getMapCenter();

    qreal centerDrift = std::abs(centerAfter.first - tapCoord.first)
                       + std::abs(centerAfter.second - tapCoord.second);
    QVERIFY2(centerDrift > 0.1,
             qPrintable(QString("Map center drifted to tap point! Distance = %1° — "
                                "double-tap should NOT recenter")
                        .arg(centerDrift, 0, 'f', 6)));

    log("testTouchDoubleTapDoesNotRecenter: PASSED — zoom-to-cursor preserves map center");
}

// ===========================================================================
// Regression tests: setZoom/setPitch in FixedFollowing browse-mode bug
// These are RED-phase tests for the three-layer trigger refactoring.
// Tests 1-2 MUST FAIL initially — proving the bug that setZoom/setPitch
// incorrectly trigger FixedBrowsing due to missing m_selfAnimating=true.
// Tests 3-8 define expected behavior after the refactoring.
// ===========================================================================

void GuiTest::testSetZoomDoesNotTriggerBrowse()
{
    log("testSetZoomDoesNotTriggerBrowse: setZoom must NOT trigger FixedBrowsing");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    // Spy on followingPausedChanged BEFORE calling setZoom so we capture
    // any synchronous or asynchronous signal emission
    QSignalSpy spy(m_locationIndicatorManager, &LocationIndicatorManager::followingPausedChanged);
    QVERIFY(spy.isValid());

    double zoomBefore = m_map->map()->zoom();
    log(QStringLiteral("Zoom before setZoom: %1").arg(zoomBefore));

    // Call setZoom synchronously after setup. The follow animation timer
    // may or may not have set m_selfAnimating=true yet. We wait long enough
    // for the render to process any pending region changes.
    m_locationIndicatorManager->setZoom(15.0);

    // Wait for map rendering to process the zoom change and emit signals
    QTest::qWait(2000);

    double zoomAfter = m_map->map()->zoom();
    log(QStringLiteral("Zoom after setZoom: %1, spy count: %2").arg(zoomAfter).arg(spy.count()));

    // RED PHASE: This should fail — setZoom triggers spurious FixedBrowsing.
    // If the test passes, the bug may be masked by the animation timer
    // (which constantly sets m_selfAnimating=true at 30fps).
    QCOMPARE(spy.count(), 0);

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testSetPitchDoesNotTriggerBrowse()
{
    log("testSetPitchDoesNotTriggerBrowse: setPitch must NOT trigger FixedBrowsing");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    QTest::qWait(1500);

    QSignalSpy spy(m_locationIndicatorManager, &LocationIndicatorManager::followingPausedChanged);
    QVERIFY(spy.isValid());

    m_locationIndicatorManager->setPitch(45.0);
    QTest::qWait(1000);

    // RED PHASE: Same bug as setZoom — missing m_selfAnimating causes spurious browse.
    QCOMPARE(spy.count(), 0);

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testUserPanDetectedTriggersBrowse()
{
    log("testUserPanDetectedTriggersBrowse: user pan signal must trigger FixedBrowsing");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    QTest::qWait(1500);

    QSignalSpy spy(m_locationIndicatorManager, &LocationIndicatorManager::followingPausedChanged);
    QVERIFY(spy.isValid());

    // After refactoring: userPanDetected → pauseFollowing → transitionToBrowsing
    // → followingPausedChanged(true). Currently pauseFollowing is a no-op in FixedFollowing.
    emit m_map->userPanDetected();
    QTest::qWait(500);

    QVERIFY2(spy.count() >= 1, "Expected followingPausedChanged(true) after userPanDetected");
    bool paused = spy.at(0).at(0).toBool();
    QVERIFY2(paused, "Expected followingPausedChanged(true)");

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testUserZoomDetectedTriggersBrowse()
{
    log("testUserZoomDetectedTriggersBrowse: user zoom signal must trigger FixedBrowsing");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    QTest::qWait(1500);

    QSignalSpy spy(m_locationIndicatorManager, &LocationIndicatorManager::followingPausedChanged);
    QVERIFY(spy.isValid());

    // After refactoring: userZoomDetected → pauseFollowing → transitionToBrowsing
    emit m_map->userZoomDetected();
    QTest::qWait(500);

    QVERIFY2(spy.count() >= 1, "Expected followingPausedChanged(true) after userZoomDetected");
    bool paused = spy.at(0).at(0).toBool();
    QVERIFY2(paused, "Expected followingPausedChanged(true)");

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testExternalMapChangeTriggersBrowse()
{
    log("testExternalMapChangeTriggersBrowse: external programmatic change triggers browse");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    QTest::qWait(1500);

    QSignalSpy spy(m_locationIndicatorManager, &LocationIndicatorManager::followingPausedChanged);
    QVERIFY(spy.isValid());

    // Simulate MapContainer calling setZoom directly on the underlying map.
    // This is an external programmatic change — RegionWillChange fires,
    // LIM detects !m_selfAnimating, transitions to FixedBrowsing.
    m_map->map()->setZoom(10.0);
    QTest::qWait(1000);

    QVERIFY2(spy.count() >= 1, "Expected followingPausedChanged(true) after external map change");
    bool paused = spy.at(0).at(0).toBool();
    QVERIFY2(paused, "Expected followingPausedChanged(true)");

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testResumeAfterBrowse()
{
    log("testResumeAfterBrowse: browse mode auto-resumes after timeout");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(400);
    m_map->setUserInteractionEnabled(false);
    QTest::qWait(2000);

    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
    QTest::qWait(1000);

    QSignalSpy spy(m_locationIndicatorManager, &LocationIndicatorManager::followingPausedChanged);
    QVERIFY(spy.isValid());

    m_locationIndicatorManager->setLocation({36.76, 3.06});
    QTest::qWait(500);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    QTest::qWait(500);

    QWidget *mapWidget = m_map->findChild<QWidget*>();
    QVERIFY(mapWidget != nullptr);
    QPoint center = mapWidget->rect().center();

    QTest::mousePress(mapWidget, Qt::LeftButton, {}, center);
    QTest::mouseMove(mapWidget, center + QPoint(200, 0));
    QTest::mouseRelease(mapWidget, Qt::LeftButton, {}, center + QPoint(200, 0));
    QTest::qWait(1000);

    QVERIFY2(spy.count() >= 1,
             QStringLiteral("Expected followingPausedChanged(true), got %1").arg(spy.count()).toUtf8());
    QVERIFY2(spy.at(0).at(0).toBool(), "Expected followingPausedChanged(true)");

    log("Waiting for auto-resume timeout...");
    QTest::qWait(4500);

    QVERIFY2(spy.count() >= 2,
             QStringLiteral("Expected followingPausedChanged(false), got %1").arg(spy.count()).toUtf8());
    QVERIFY2(!spy.at(spy.count() - 1).at(0).toBool(), "Expected followingPausedChanged(false)");

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testFollowAnimationStillWorks()
{
    log("testFollowAnimationStillWorks: follow animation updates map center");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    m_locationIndicatorManager->setFixedHeadingMode(
        LocationIndicatorManager::FixedHeadingMode::HeadingUp);
    QTest::qWait(2000);

    // Record initial center
    QMapLibre::Coordinate before = getMapCenter();
    log(QStringLiteral("Before follow: lat=%1 lon=%2").arg(before.first).arg(before.second));

    // Send multiple GPS updates moving northeast
    double lat = 36.76;
    double lon = 3.06;
    for (int i = 0; i < 3; ++i) {
        m_locationIndicatorManager->setLocation({lat, lon});
        lat += 0.001;
        lon += 0.001;
        QTest::qWait(1500);
    }

    QMapLibre::Coordinate after = getMapCenter();
    log(QStringLiteral("After follow: lat=%1 lon=%2").arg(after.first).arg(after.second));

    double latDiff = qAbs(after.first - before.first);
    double lonDiff = qAbs(after.second - before.second);
    QVERIFY2(latDiff > 0.001 || lonDiff > 0.001,
             "Map center should have moved during follow animation");

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testSetZoomThenDrag()
{
    log("testSetZoomThenDrag: setZoom in FixedFollowing + drag must enter FixedBrowsing");

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
    QTest::qWait(1500);

    QSignalSpy spy(m_locationIndicatorManager, &LocationIndicatorManager::followingPausedChanged);
    QVERIFY(spy.isValid());

    // Step 1: Call setZoom (currently buggy — triggers browse)
    m_locationIndicatorManager->setZoom(15.0);
    QTest::qWait(1000);

    // Step 2: Simulate mouse drag — must enter FixedBrowsing
    // (After the fix, setZoom won't trigger browse; drag will)
    QWidget *mapWidget = m_map->findChild<QWidget*>();
    QVERIFY(mapWidget != nullptr);
    QPoint center = mapWidget->rect().center();

    QTest::mousePress(mapWidget, Qt::LeftButton, {}, center);
    QTest::mouseMove(mapWidget, center + QPoint(200, 0));
    QTest::mouseRelease(mapWidget, Qt::LeftButton, {}, center + QPoint(200, 0));
    QTest::qWait(1000);

    // At least one followingPausedChanged(true) should have been emitted
    // (from setZoom in current buggy code, or from drag after the fix)
    QVERIFY2(spy.count() >= 1, "Expected followingPausedChanged(true) after setZoom+drag");

    // Verify the last signal was followingPausedChanged(true)
    bool lastPaused = spy.at(spy.count() - 1).at(0).toBool();
    QVERIFY2(lastPaused, "Expected followingPausedChanged(true) — browsing active after drag");

    // Wait for resume
    log("Waiting for auto-resume...");
    QTest::qWait(4500);

    QVERIFY2(spy.count() >= 2, "Expected followingPausedChanged(false) after resume");
    bool resumed = !spy.at(spy.count() - 1).at(0).toBool();
    QVERIFY2(resumed, "Expected followingPausedChanged(false)");

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testFollowBearingChangedHeadingUp()
{
    QSignalSpy spy(m_locationIndicatorManager, &LocationIndicatorManager::followBearingChanged);
    QVERIFY(spy.isValid());

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
    m_locationIndicatorManager->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::HeadingUp);
    m_locationIndicatorManager->setLocation({36.75, 3.05, 90.0});
    m_locationIndicatorManager->setZoom(15.0);
    m_locationIndicatorManager->setPitch(45.0);
    QTest::qWait(2000);

    QVERIFY2(spy.count() > 5, QStringLiteral("Expected multiple followBearingChanged emits during HeadingUp animation, got %1").arg(spy.count()).toUtf8());
    if (spy.count() > 0) {
        double lastBearing = spy.at(spy.count()-1).at(0).value<double>();
        QVERIFY2(qAbs(lastBearing - 90.0) < 10.0, QStringLiteral("Last bearing should be ~90, got %1").arg(lastBearing).toUtf8());
    }

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testFollowBearingChangedNorthUpDrift()
{
    QSignalSpy spy(m_locationIndicatorManager, &LocationIndicatorManager::followBearingChanged);
    QVERIFY(spy.isValid());

    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
    m_locationIndicatorManager->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::NorthUp);

    double bearingBefore = m_map->map()->bearing();
    m_locationIndicatorManager->setLocation({36.75, 3.05, 90.0});
    m_locationIndicatorManager->setZoom(15.0);
    m_locationIndicatorManager->setPitch(45.0);
    QTest::qWait(2000);

    QVERIFY2(spy.count() > 0, QStringLiteral("Expected followBearingChanged emit during NorthUp drift correction, got %1").arg(spy.count()).toUtf8());
    if (spy.count() > 0) {
        double lastBearing = spy.at(spy.count()-1).at(0).value<double>();
        QVERIFY2(qAbs(lastBearing) < 5.0, QStringLiteral("Last bearing should be near 0, got %1").arg(lastBearing).toUtf8());
    }

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testFollowBearingChangedOnModeSwitch()
{
    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
    m_locationIndicatorManager->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::HeadingUp);
    m_locationIndicatorManager->setLocation({36.75, 3.05, 90.0});
    QTest::qWait(2000);

    QSignalSpy spy(m_locationIndicatorManager, &LocationIndicatorManager::followBearingChanged);
    QVERIFY(spy.isValid());

    m_locationIndicatorManager->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::NorthUp);
    QTest::qWait(500);

    QVERIFY2(spy.count() >= 1, QStringLiteral("Expected followBearingChanged emit on mode switch to NorthUp, got %1").arg(spy.count()).toUtf8());
    if (spy.count() > 0) {
        double lastBearing = spy.at(spy.count()-1).at(0).value<double>();
        QVERIFY2(qAbs(lastBearing) < 0.01, QStringLiteral("Bearing should be 0.0 after switching to NorthUp, got %1").arg(lastBearing).toUtf8());
    }

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

void GuiTest::testMapContainerBearingBridgedAndLastBearingSynced()
{
    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::blue);
    m_locationIndicatorManager->setLocationIcon(icon);
    m_locationIndicatorManager->setLocation({36.75, 3.05});
    m_locationIndicatorManager->showLocation();
    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Fixed);
    m_locationIndicatorManager->setCenterOffset(200);
    m_map->setUserInteractionEnabled(true);
    m_locationIndicatorManager->setFixedTouchResumeTimeout(3000);
    m_locationIndicatorManager->setFixedHeadingMode(LocationIndicatorManager::FixedHeadingMode::HeadingUp);
    m_locationIndicatorManager->setLocation({36.75, 3.05, 90.0});
    m_locationIndicatorManager->setZoom(15.0);
    m_locationIndicatorManager->setPitch(45.0);
    QTest::qWait(2000);

    QSignalSpy spy(m_map, &MapContainer::bearingChanged);
    QVERIFY(spy.isValid());
    spy.clear();

    double cur = m_map->map()->zoom();
    m_map->map()->setZoom(cur + 0.5);
    QTest::qWait(500);
    m_map->map()->setZoom(cur);
    QTest::qWait(500);

    QVERIFY2(spy.count() == 0, QStringLiteral("AC10 FAIL: bearingChanged re-emitted after unrelated mapChanged (m_lastBearing not synced?). Got %1 emits").arg(spy.count()).toUtf8());

    m_locationIndicatorManager->setMode(LocationIndicatorManager::LocationMode::Free);
    m_locationIndicatorManager->hideLocation();
    QTest::qWait(500);
}

QTEST_MAIN(GuiTest)
#include "tst_gui.moc"
