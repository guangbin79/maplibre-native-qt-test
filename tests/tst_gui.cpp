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
    void testAutoRunSequence();
    void testFixedModePanBlocked();
    void testFixedModePanAllowed();

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
    QString rootPath = "/home/guangbin/Documents/untitled/build/linux-x86_64-test/map_data";
    g_server = new HXGISServer("127.0.0.1:4943", rootPath.toUtf8().constData());
    QVERIFY2(g_server->isRunning(), "Failed to start HXGISServer");
    log(QStringLiteral("HXGISServer started, version: %1").arg(g_server->version()));

    QTest::qWait(500);

    log("initTestCase: creating MainWindow");
    m_window = new MainWindow();
    m_window->show();
    m_window->resize(1200, 800);

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

    m_map->setAnnotations(anns, icons);
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
    m_map->setCenterOffset(400);
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
    m_map->setFixedTouchResumeTimeout(3000);
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

QTEST_MAIN(GuiTest)
#include "tst_gui.moc"
