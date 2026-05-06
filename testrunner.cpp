#include "testrunner.h"
#include "mapcontainer.h"
#include <QDebug>
#include <QPainter>
#include <QRandomGenerator>

TestRunner::TestRunner(MapContainer *mapContainer, QObject *parent)
    : QObject(parent)
    , m_mapContainer(mapContainer)
{
    m_testIcons["marker"] = createTestIcon(QColor(255, 0, 0));
    m_testIcons["pin"] = createTestIcon(QColor(0, 255, 0));
    m_testIcons["star"] = createTestIcon(QColor(0, 0, 255));
}

void TestRunner::startTests()
{
    if (m_running) {
        log("INFO", "Tests already running, ignoring start request");
        return;
    }

    m_running = true;
    m_currentStep = Step_Annotation_AddSingle;
    m_passedCount = 0;
    m_failedCount = 0;
    m_totalCount = 0;
    m_testTimer.start();

    log("INFO", "========================================");
    log("INFO", "  Device Auto-Test Started");
    log("INFO", "========================================");

    emit testsStarted();
    runNextStep();
}

void TestRunner::runNextStep()
{
    if (!m_running)
        return;

    executeStep(m_currentStep);
}

void TestRunner::executeStep(TestStep step)
{
    switch (step) {
    // ============================================================
    // 地图基础操作测试
    // ============================================================
    // 验证 MapContainer 的 zoom/bearing/pitch/center API 是否正常工作。
    // 每步操作后等待 1500ms，方便在设备上观察地图变化。
    // 这些测试不验证返回值（MapLibre 的动画是异步的），只验证 API 调用不崩溃。
    // ============================================================

    case Step_Map_ZoomOut:
        logTestStart("Map - Zoom Out");
        {
            // 从默认 Z8 缩小到 Z6，观察更大范围的地图（国家级别视图）
            m_mapContainer->setZoom(6.0);
            logTestPass("Zoomed out to Z6");
        }
        nextStep();
        break;

    case Step_Map_ZoomIn:
        logTestStart("Map - Zoom In");
        {
            // 从 Z6 放大到 Z12，观察城市街道级别细节
            m_mapContainer->setZoom(12.0);
            logTestPass("Zoomed in to Z12");
        }
        nextStep();
        break;

    case Step_Map_ZoomReset:
        logTestStart("Map - Zoom Reset");
        {
            // 恢复默认缩放级别 Z8（城市级别视图）
            m_mapContainer->setZoom(8.0);
            logTestPass("Zoom reset to Z8");
        }
        nextStep();
        break;

    case Step_Map_Rotate45:
        logTestStart("Map - Rotate 45");
        {
            // 顺时针旋转 45°，验证 bearing API
            m_mapContainer->setBearing(45.0);
            logTestPass("Rotated to 45 degrees");
        }
        nextStep();
        break;

    case Step_Map_Rotate90:
        logTestStart("Map - Rotate 90");
        {
            // 继续旋转到 90°，地图将 sideways 显示
            m_mapContainer->setBearing(90.0);
            logTestPass("Rotated to 90 degrees");
        }
        nextStep();
        break;

    case Step_Map_RotateReset:
        logTestStart("Map - Rotate Reset");
        {
            // 恢复正北朝上
            m_mapContainer->setBearing(0.0);
            logTestPass("Rotation reset to 0 degrees");
        }
        nextStep();
        break;

    case Step_Map_PanToShanghai:
        logTestStart("Map - Pan to Shanghai");
        {
            // 平移到上海坐标 (31.23°N, 121.47°E)，验证 setCenter API
            m_mapContainer->setCenter(31.23, 121.47);
            logTestPass("Panned to Shanghai (31.23, 121.47)");
        }
        nextStep();
        break;

    case Step_Map_PanToBeijing:
        logTestStart("Map - Pan to Beijing");
        {
            // 平移到北京坐标 (39.90°N, 116.41°E)
            m_mapContainer->setCenter(39.90, 116.41);
            logTestPass("Panned to Beijing (39.90, 116.41)");
        }
        nextStep();
        break;

    case Step_Map_PanReset:
        logTestStart("Map - Pan Reset");
        {
            // 恢复初始位置阿尔及尔 (36.75°N, 3.05°E)
            m_mapContainer->setCenter(36.75, 3.05);
            logTestPass("Panned back to Algiers (36.75, 3.05)");
        }
        nextStep();
        break;

    case Step_Map_Tilt30:
        logTestStart("Map - Tilt 30");
        {
            // 设置倾斜角度 30°，启用 3D 透视效果（需样式支持 terrain）
            m_mapContainer->setPitch(30.0);
            logTestPass("Tilt set to 30 degrees");
        }
        nextStep();
        break;

    case Step_Map_Tilt60:
        logTestStart("Map - Tilt 60");
        {
            // 最大倾斜角度 60°（MapContainer 内部限制 MAX_PITCH = 60.0）
            m_mapContainer->setPitch(60.0);
            logTestPass("Tilt set to 60 degrees");
        }
        nextStep();
        break;

    case Step_Map_TiltReset:
        logTestStart("Map - Tilt Reset");
        {
            // 恢复垂直俯视视角
            m_mapContainer->setPitch(0.0);
            logTestPass("Tilt reset to 0 degrees");
        }
        nextStep();
        break;

    case Step_Annotation_AddSingle:
        logTestStart("Annotation - Add Single");
        {
            MapAnnotation ann = createAnnotation("test-ann-1", 39.9042, 116.4074,
                                                    QStringLiteral("天安门"), QStringLiteral("marker"));
            m_mapContainer->addAnnotation(ann, m_testIcons["marker"]);
            QStringList ids = m_mapContainer->allIds();
            if (ids.contains("test-ann-1")) {
                logTestPass("Single annotation added successfully");
            } else {
                logTestFail("Single annotation not found after add");
            }
        }
        nextStep();
        break;

    case Step_Annotation_VerifySingle:
        logTestStart("Annotation - Verify Visible");
        {
            QStringList visible = m_mapContainer->visibleIds();
            if (visible.contains("test-ann-1")) {
                logTestPass("Annotation is visible");
            } else {
                logTestFail("Annotation not visible after add");
            }
        }
        nextStep();
        break;

    case Step_Annotation_AddBatch:
        logTestStart("Annotation - Add Batch");
        {
            QVector<MapAnnotation> anns;
            anns.append(createAnnotation("test-ann-2", 31.2304, 121.4737,
                                          QStringLiteral("外滩"), QStringLiteral("pin")));
            anns.append(createAnnotation("test-ann-3", 23.1291, 113.2644,
                                          QStringLiteral("广州塔"), QStringLiteral("star")));
            m_mapContainer->addAnnotations(anns, m_testIcons);

            QStringList ids = m_mapContainer->allIds();
            if (ids.size() == 3) {
                logTestPass(QStringLiteral("Batch added, total: %1 annotations").arg(ids.size()));
            } else {
                logTestFail(QStringLiteral("Expected 3 annotations, got %1").arg(ids.size()));
            }
        }
        nextStep();
        break;

    case Step_Annotation_VerifyBatch:
        logTestStart("Annotation - Verify Batch Visible");
        {
            QStringList visible = m_mapContainer->visibleIds();
            if (visible.size() == 3) {
                logTestPass(QStringLiteral("All %1 annotations visible").arg(visible.size()));
            } else {
                logTestFail(QStringLiteral("Expected 3 visible, got %1").arg(visible.size()));
            }
        }
        nextStep();
        break;

    case Step_Annotation_Hide:
        logTestStart("Annotation - Hide All");
        {
            m_mapContainer->hideAllAnnotations();
            QStringList visible = m_mapContainer->visibleIds();
            if (visible.isEmpty()) {
                logTestPass("All annotations hidden");
            } else {
                logTestFail(QStringLiteral("Expected 0 visible, got %1").arg(visible.size()));
            }
        }
        nextStep();
        break;

    case Step_Annotation_Show:
        logTestStart("Annotation - Show All");
        {
            m_mapContainer->showAllAnnotations();
            QStringList visible = m_mapContainer->visibleIds();
            if (visible.size() == 3) {
                logTestPass("All annotations shown again");
            } else {
                logTestFail(QStringLiteral("Expected 3 visible, got %1").arg(visible.size()));
            }
        }
        nextStep();
        break;

    case Step_Annotation_RemoveSingle:
        logTestStart("Annotation - Remove Single");
        {
            m_mapContainer->removeAnnotation("test-ann-1");
            QStringList ids = m_mapContainer->allIds();
            if (!ids.contains("test-ann-1") && ids.size() == 2) {
                logTestPass("Single annotation removed");
            } else {
                logTestFail(QStringLiteral("Remove failed, remaining: %1").arg(ids.size()));
            }
        }
        nextStep();
        break;

    case Step_Annotation_Clear:
        logTestStart("Annotation - Clear All");
        {
            m_mapContainer->clearAnnotations();
            QStringList ids = m_mapContainer->allIds();
            if (ids.isEmpty()) {
                logTestPass("All annotations cleared");
            } else {
                logTestFail(QStringLiteral("Expected 0, got %1").arg(ids.size()));
            }
        }
        nextStep();
        break;

    case Step_Route_AddSingle:
        logTestStart("Route - Add Single");
        {
            QVector<QPair<double, double>> coords;
            coords.append(qMakePair(39.9042, 116.4074));
            coords.append(qMakePair(39.9156, 116.3974));
            MapRouteSegment seg = createRouteSegment("test-route-1", "route-A",
                                                        coords, QColor(255, 0, 0), 3.0, false);
            m_mapContainer->addRouteSegment(seg);

            QStringList ids = m_mapContainer->allRouteIds();
            if (ids.contains("route-A")) {
                logTestPass("Single route added");
            } else {
                logTestFail("Route not found after add");
            }
        }
        nextStep();
        break;

    case Step_Route_VerifySingle:
        logTestStart("Route - Verify Visible");
        {
            QStringList visible = m_mapContainer->visibleRouteIds();
            if (visible.contains("route-A")) {
                logTestPass("Route is visible");
            } else {
                logTestFail("Route not visible after add");
            }
        }
        nextStep();
        break;

    case Step_Route_AddBatch:
        logTestStart("Route - Add Batch");
        {
            QVector<MapRouteSegment> segs;

            QVector<QPair<double, double>> coords1;
            coords1.append(qMakePair(31.2304, 121.4737));
            coords1.append(qMakePair(31.2404, 121.4837));
            segs.append(createRouteSegment("test-route-2", "route-B",
                                              coords1, QColor(0, 255, 0), 2.0, true));

            QVector<QPair<double, double>> coords2;
            coords2.append(qMakePair(23.1291, 113.2644));
            coords2.append(qMakePair(23.1391, 113.2744));
            segs.append(createRouteSegment("test-route-3", "route-C",
                                              coords2, QColor(0, 0, 255), 4.0, false));

            m_mapContainer->addRouteSegments(segs);

            QStringList ids = m_mapContainer->allRouteIds();
            if (ids.size() == 3) {
                logTestPass(QStringLiteral("Batch added, total: %1 routes").arg(ids.size()));
            } else {
                logTestFail(QStringLiteral("Expected 3 routes, got %1").arg(ids.size()));
            }
        }
        nextStep();
        break;

    case Step_Route_VerifyBatch:
        logTestStart("Route - Verify Batch Visible");
        {
            QStringList visible = m_mapContainer->visibleRouteIds();
            if (visible.size() == 3) {
                logTestPass(QStringLiteral("All %1 routes visible").arg(visible.size()));
            } else {
                logTestFail(QStringLiteral("Expected 3 visible, got %1").arg(visible.size()));
            }
        }
        nextStep();
        break;

    case Step_Route_Hide:
        logTestStart("Route - Hide All");
        {
            m_mapContainer->hideAllRoutes();
            QStringList visible = m_mapContainer->visibleRouteIds();
            if (visible.isEmpty()) {
                logTestPass("All routes hidden");
            } else {
                logTestFail(QStringLiteral("Expected 0 visible, got %1").arg(visible.size()));
            }
        }
        nextStep();
        break;

    case Step_Route_Show:
        logTestStart("Route - Show All");
        {
            m_mapContainer->showAllRoutes();
            QStringList visible = m_mapContainer->visibleRouteIds();
            if (visible.size() == 3) {
                logTestPass("All routes shown again");
            } else {
                logTestFail(QStringLiteral("Expected 3 visible, got %1").arg(visible.size()));
            }
        }
        nextStep();
        break;

    case Step_Route_RemoveSingle:
        logTestStart("Route - Remove Single Segment");
        {
            m_mapContainer->removeRouteSegment("test-route-1");
            QStringList ids = m_mapContainer->allRouteIds();
            logTestPass("Single segment removed");
        }
        nextStep();
        break;

    case Step_Route_Clear:
        logTestStart("Route - Clear All");
        {
            m_mapContainer->clearRoutes();
            QStringList ids = m_mapContainer->allRouteIds();
            if (ids.isEmpty()) {
                logTestPass("All routes cleared");
            } else {
                logTestFail(QStringLiteral("Expected 0, got %1").arg(ids.size()));
            }
        }
        nextStep();
        break;

    case Step_Location_SetPosition:
        logTestStart("Location - Set Position");
        {
            m_mapContainer->setLocation(39.9042, 116.4074);
            logTestPass("Location set to Beijing");
        }
        nextStep();
        break;

    case Step_Location_Show:
        logTestStart("Location - Show");
        {
            m_mapContainer->setLocationIcon(createTestIcon(QColor(0, 128, 255)));
            m_mapContainer->showLocation();
            if (m_mapContainer->isLocationVisible()) {
                logTestPass("Location indicator shown");
            } else {
                logTestFail("Location indicator not visible after show");
            }
        }
        nextStep();
        break;

    case Step_Location_Hide:
        logTestStart("Location - Hide");
        {
            m_mapContainer->hideLocation();
            if (!m_mapContainer->isLocationVisible()) {
                logTestPass("Location indicator hidden");
            } else {
                logTestFail("Location indicator still visible after hide");
            }
        }
        nextStep();
        break;

    case Step_Location_ModeFixed:
        logTestStart("Location - Switch to Fixed Mode");
        {
            m_mapContainer->showLocation();
            m_mapContainer->setCenterOffset(200);
            m_mapContainer->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
            m_mapContainer->setLocation(39.9042, 116.4074);
            if (m_mapContainer->locationMode() == LocationIndicatorManager::LocationMode::Fixed) {
                logTestPass("Switched to Fixed mode");
            } else {
                logTestFail("Failed to switch to Fixed mode");
            }
        }
        nextStep();
        break;

    case Step_Location_ModeFree:
        logTestStart("Location - Switch to Free Mode");
        {
            m_mapContainer->setLocationMode(LocationIndicatorManager::LocationMode::Free);
            if (m_mapContainer->locationMode() == LocationIndicatorManager::LocationMode::Free) {
                logTestPass("Switched to Free mode");
            } else {
                logTestFail("Failed to switch to Free mode");
            }
        }
        nextStep();
        break;

    case Step_Location_Rotation:
        logTestStart("Location - Set Rotation");
        {
            m_mapContainer->setLocationRotation(45.0);
            if (qFuzzyCompare(m_mapContainer->locationRotation(), 45.0)) {
                logTestPass("Rotation set to 45 degrees");
            } else {
                logTestFail(QStringLiteral("Rotation mismatch: %1").arg(m_mapContainer->locationRotation()));
            }
        }
        nextStep();
        break;

    case Step_Location_CenterOffset:
        logTestStart("Location - Center Offset");
        {
            m_mapContainer->setCenterOffset(300);
            logTestPass("Center offset set to 300px");
        }
        nextStep();
        break;

    case Step_Combo_AnnotationRoute:
        logTestStart("Combo - Annotation + Route Together");
        {
            QVector<MapAnnotation> anns;
            anns.append(createAnnotation("combo-ann-1", 39.9, 116.4,
                                          QStringLiteral("Combo Point"), QStringLiteral("marker")));
            m_mapContainer->setAnnotations(anns, m_testIcons);

            QVector<MapRouteSegment> segs;
            QVector<QPair<double, double>> coords;
            coords.append(qMakePair(39.9, 116.4));
            coords.append(qMakePair(39.91, 116.41));
            segs.append(createRouteSegment("combo-route-1", "combo-route",
                                              coords, QColor(255, 128, 0), 2.0, false));
            m_mapContainer->setRoutes(segs);

            QStringList annIds = m_mapContainer->allIds();
            QStringList routeIds = m_mapContainer->allRouteIds();
            if (annIds.size() == 1 && routeIds.size() == 1) {
                logTestPass("Annotation and route displayed together");
            } else {
                logTestFail(QStringLiteral("Combo failed: ann=%1 route=%2").arg(annIds.size()).arg(routeIds.size()));
            }
        }
        nextStep();
        break;

    case Step_Combo_LayerToggle:
        logTestStart("Combo - Layer Toggle Independence");
        {
            m_mapContainer->hideAllAnnotations();
            QStringList annVisible = m_mapContainer->visibleIds();
            QStringList routeVisible = m_mapContainer->visibleRouteIds();
            if (annVisible.isEmpty() && routeVisible.size() == 1) {
                logTestPass("Annotation hidden, route still visible");
            } else {
                logTestFail("Layer toggle independence failed");
            }

            m_mapContainer->showAllAnnotations();
            m_mapContainer->clearAnnotations();
            m_mapContainer->clearRoutes();
        }
        nextStep();
        break;

    case Step_Finished:
        log("INFO", "========================================");
        log("INFO", QStringLiteral("  Test Run Complete"));
        log("INFO", QStringLiteral("  Total:  %1").arg(m_totalCount));
        log("INFO", QStringLiteral("  Passed: %1").arg(m_passedCount));
        log("INFO", QStringLiteral("  Failed: %1").arg(m_failedCount));
        log("INFO", QStringLiteral("  Time:   %1 ms").arg(m_testTimer.elapsed()));
        log("INFO", "========================================");

        emit allTestsFinished(m_totalCount, m_passedCount, m_failedCount, m_testTimer.elapsed());
        m_running = false;
        m_currentStep = Step_Idle;
        break;

    default:
        m_running = false;
        m_currentStep = Step_Idle;
        break;
    }
}

void TestRunner::nextStep()
{
    m_currentStep = static_cast<TestStep>(static_cast<int>(m_currentStep) + 1);

    if (m_currentStep == Step_Finished) {
        runNextStep();
    } else if (m_currentStep < Step_Finished) {
        QTimer::singleShot(m_stepDelayMs, this, &TestRunner::runNextStep);
    }
}

void TestRunner::log(const QString &level, const QString &message)
{
    QString formatted = QStringLiteral("[TEST] [%1] %2").arg(level, message);
    qDebug().noquote() << formatted;
    emit logMessage(level, message);
}

void TestRunner::logTestStart(const QString &testName)
{
    m_currentTestName = testName;
    m_totalCount++;
    log("STEP", QStringLiteral("===== %1 =====").arg(testName));
}

void TestRunner::logTestPass(const QString &message)
{
    m_passedCount++;
    if (message.isEmpty()) {
        log("PASS", QStringLiteral("%1: PASS").arg(m_currentTestName));
    } else {
        log("PASS", QStringLiteral("%1: PASS - %2").arg(m_currentTestName, message));
    }
    emit testCaseFinished(m_currentTestName, true, message);
}

void TestRunner::logTestFail(const QString &message)
{
    m_failedCount++;
    log("FAIL", QStringLiteral("%1: FAIL - %2").arg(m_currentTestName, message));
    emit testCaseFinished(m_currentTestName, false, message);
}

MapAnnotation TestRunner::createAnnotation(const QString &id, double lat, double lon,
                                            const QString &title, const QString &iconName)
{
    MapAnnotation ann;
    ann.id = id;
    ann.latitude = lat;
    ann.longitude = lon;
    ann.title = title;
    ann.iconName = iconName;
    return ann;
}

MapRouteSegment TestRunner::createRouteSegment(const QString &id, const QString &routeId,
                                                const QVector<QPair<double, double>> &coords,
                                                const QColor &color, double width, bool dashed)
{
    MapRouteSegment seg;
    seg.id = id;
    seg.routeId = routeId;
    for (const auto &c : coords) {
        seg.coordinates.append(qMakePair(c.first, c.second));
    }
    seg.color = color;
    seg.width = width;
    seg.dashed = dashed;
    return seg;
}

QImage TestRunner::createTestIcon(const QColor &color)
{
    QImage icon(32, 32, QImage::Format_ARGB32);
    icon.fill(Qt::transparent);

    QPainter painter(&icon);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, 28, 28);
    painter.end();

    return icon;
}
