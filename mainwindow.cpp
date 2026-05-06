#include "mainwindow.h"
#include "mapcontainer.h"
#include "scalebarwidget.h"
#include "controlpanelwidget.h"
#include "testrunner.h"
#include <QMapLibre/Map>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTextBrowser>
#include <QResizeEvent>
#include <QTimer>
#include <QStandardPaths>

/**
 * @brief 主窗口构造函数 - UI布局与信号连接初始化
 *
 * 初始化流程：
 * 1. 创建中央部件和水平布局
 * 2. 创建三个核心组件并添加到布局
 * 3. 建立组件间的信号-槽连接
 *
 * 跨平台说明：
 * 本程序通过 Q_OS_ANDROID 宏区分桌面和 Android 平台，自动适配：
 *   - 服务器地址：桌面用 127.0.0.1（本地 HXGISServer），Android 用远程服务器 IP
 *   - 数据路径：桌面用绝对路径，Android 用应用私有目录
 *   - UI 尺寸：Android 按钮更大（最小 48dp），字体更大
 * 打包到 APK 时，需将 map_data 放入 assets 或 SD 卡对应目录。
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mapContainer(nullptr)
    , m_scaleBar(nullptr)
    , m_controlPanel(nullptr)
    , m_annotationLayerToggle(nullptr)
    , m_routeLayerToggle(nullptr)
    , m_locationLayerToggle(nullptr)
    , m_testRunner(nullptr)
    , m_testLogView(nullptr)
{
    setWindowTitle(QStringLiteral("Map Viewer"));

    // ============================================================
    // 跨平台：窗口尺寸
    // 桌面默认 1200x800，Android 全屏（由系统控制）
    // ============================================================
#ifndef Q_OS_ANDROID
    resize(1200, 800);
#endif

    // ============================================================
    // 跨平台：配置参数
    // ============================================================
#ifdef Q_OS_ANDROID
    // Android：HXGISServer 运行在远程主机，需改为实际 IP
    // 调试时可使用 adb reverse tcp:4943 tcp:4943 将桌面端口转发到设备
    const QString serverHost = QStringLiteral("192.168.1.100");
    const QString serverUrl = QStringLiteral("http://%1:4943/styles/day/style.json?schema=hxmap").arg(serverHost);

    // Android：数据目录为应用私有目录 /sdcard/Android/data/<pkg>/files/map_data
    // 首次运行需将 map_data 复制到该目录（可通过 Qt 的 assets 或手动 push）
    const QString dataRootPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/map_data");
#else
    // 桌面：HXGISServer 本地运行
    const QString serverHost = QStringLiteral("127.0.0.1");
    const QString serverUrl = QStringLiteral("http://127.0.0.1:4943/styles/day/style.json?schema=hxmap");

    // 桌面：数据目录为构建目录下的绝对路径
    const QString dataRootPath = QStringLiteral("/home/guangbin/Documents/untitled/build/linux-x86_64-test/map_data");
#endif

    // ============================================================
    // 跨平台：UI 尺寸常量
    // Android 遵循 Material Design 规范，按钮最小 48dp，字体更大
    // ============================================================
#ifdef Q_OS_ANDROID
    const int panelWidth = 180;           // 控制面板宽度（Android 需要更宽）
    const int btnFontSize = 12;           // 按钮字体大小
    const int btnPadding = 6;             // 按钮内边距
    const int logViewMaxHeight = 160;     // 日志区域最大高度
    const int logFontSize = 11;           // 日志字体大小
#else
    const int panelWidth = 150;           // 控制面板宽度
    const int btnFontSize = 10;           // 按钮字体大小
    const int btnPadding = 2;             // 按钮内边距
    const int logViewMaxHeight = 120;     // 日志区域最大高度
    const int logFontSize = 10;           // 日志字体大小
#endif

    // ── 中央部件设置 ────────────────────────────────────────
    auto *central = new QWidget(this);
    setCentralWidget(central);

    // ── UI 布局结构 ─────────────────────────────────────────
    // 使用 QHBoxLayout 实现左右分栏：
    // ┌──────────────────────────────┬──────────────┐
    // │ MapContainer (stretch=1)     │ ControlPanel │
    // │ 自适应填充剩余空间            │ fixed width  │
    // └──────────────────────────────┴──────────────┘
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 地图容器 - 占据左侧所有可用空间 (stretch=1)
    MapContainer::MapConfig config;
    config.styleUrl = serverUrl;
    m_mapContainer = new MapContainer(config, central);
    layout->addWidget(m_mapContainer, 1);

    // 启用 Fixed 模式下的触摸平移暂停功能：
    // 用户在 Fixed 模式下单指拖动地图时，自动暂停 Fixed 跟随（切换到 Free 显示），
    // 松手后经过超时时间自动恢复 Fixed 模式。
    m_mapContainer->setFixedTouchPanEnabled(false);
    m_mapContainer->setFixedTouchResumeTimeout(3000);

    // 控制面板 - 固定宽度，容纳 API 演示按钮 (stretch=0)
    m_controlPanel = new ControlPanelWidget(central);
    m_controlPanel->setFixedWidth(panelWidth);
    layout->addWidget(m_controlPanel, 0);

    // 比例尺 - 作为 MapContainer 的子部件，定位在左下角
    m_scaleBar = new ScaleBarWidget(m_mapContainer);
    repositionScaleBar();

    // 标注图层开关 - 在控制面板中添加复选框
    m_annotationLayerToggle = new QCheckBox(QStringLiteral("标注"), m_controlPanel);
    m_annotationLayerToggle->setChecked(true);
    m_annotationLayerToggle->setStyleSheet(QStringLiteral("color: white; font-size: 11px;"));
    m_controlPanel->layout()->addWidget(m_annotationLayerToggle);

    m_routeLayerToggle = new QCheckBox(QStringLiteral("线路"), m_controlPanel);
    m_routeLayerToggle->setChecked(true);
    m_routeLayerToggle->setStyleSheet(QStringLiteral("color: white; font-size: 11px;"));
    m_controlPanel->layout()->addWidget(m_routeLayerToggle);

    m_locationLayerToggle = new QCheckBox(QStringLiteral("位置"), m_controlPanel);
    m_locationLayerToggle->setChecked(false);
    m_locationLayerToggle->setStyleSheet(QStringLiteral("color: white; font-size: 11px;"));
    m_controlPanel->layout()->addWidget(m_locationLayerToggle);

    // ============================================================
    // API 演示按钮区域 - 供开发人员手动测试各接口
    // 每个按钮对应一个 MapContainer API 调用，点击即可观察效果
    // 所有 API 调用均带有详细注释说明参数含义和使用场景
    // ============================================================

    // ── 缩放 API 演示 ──
    // setZoom(double zoom) - 设置地图缩放级别
    //   zoom: 缩放级别，范围通常为 0-22
    //   0=全球视图, 5=国家, 10=城市, 15=街道, 20=建筑物
    auto *btnZoomOut = new QPushButton(QStringLiteral("缩小 (Z6)"), m_controlPanel);
    btnZoomOut->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #2196F3; color: white; font-size: %1px; padding: %2px; }"
    ).arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnZoomOut);
    connect(btnZoomOut, &QPushButton::clicked, this, [this]() {
        // 缩小到 Z6，观察国家级别视图
        m_mapContainer->setZoom(6.0);
    });

    auto *btnZoomIn = new QPushButton(QStringLiteral("放大 (Z12)"), m_controlPanel);
    btnZoomIn->setStyleSheet(QStringLiteral("QPushButton { background-color: #2196F3; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnZoomIn);
    connect(btnZoomIn, &QPushButton::clicked, this, [this]() {
        // 放大到 Z12，观察街道级别细节
        m_mapContainer->setZoom(12.0);
    });

    auto *btnZoomReset = new QPushButton(QStringLiteral("缩放复位"), m_controlPanel);
    btnZoomReset->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnZoomReset);
    connect(btnZoomReset, &QPushButton::clicked, this, [this]() {
        // 恢复默认缩放级别 Z8
        m_mapContainer->setZoom(8.0);
    });

    // ── 旋转 API 演示 ──
    // setBearing(double bearing) - 设置地图旋转角度（方位角）
    //   bearing: 顺时针旋转角度，单位度。0=正北朝上
    auto *btnRotate45 = new QPushButton(QStringLiteral("旋转 45°"), m_controlPanel);
    btnRotate45->setStyleSheet(QStringLiteral("QPushButton { background-color: #FF9800; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnRotate45);
    connect(btnRotate45, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setBearing(45.0);
    });

    auto *btnRotate90 = new QPushButton(QStringLiteral("旋转 90°"), m_controlPanel);
    btnRotate90->setStyleSheet(QStringLiteral("QPushButton { background-color: #FF9800; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnRotate90);
    connect(btnRotate90, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setBearing(90.0);
    });

    auto *btnRotateReset = new QPushButton(QStringLiteral("旋转复位"), m_controlPanel);
    btnRotateReset->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnRotateReset);
    connect(btnRotateReset, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setBearing(0.0);
    });

    // ── 平移 API 演示 ──
    // setCenter(double lat, double lon) - 设置地图中心坐标
    //   lat: 纬度 [-90, 90], lon: 经度 [-180, 180]
    //   注意：坐标顺序是 (lat, lon)，与 GeoJSON (lon, lat) 不同
    auto *btnPanShanghai = new QPushButton(QStringLiteral("平移→上海"), m_controlPanel);
    btnPanShanghai->setStyleSheet(QStringLiteral("QPushButton { background-color: #9C27B0; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnPanShanghai);
    connect(btnPanShanghai, &QPushButton::clicked, this, [this]() {
        // 平移到上海 (31.23°N, 121.47°E)
        m_mapContainer->setCenter(31.23, 121.47);
    });

    auto *btnPanBeijing = new QPushButton(QStringLiteral("平移→北京"), m_controlPanel);
    btnPanBeijing->setStyleSheet(QStringLiteral("QPushButton { background-color: #9C27B0; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnPanBeijing);
    connect(btnPanBeijing, &QPushButton::clicked, this, [this]() {
        // 平移到北京 (39.90°N, 116.41°E)
        m_mapContainer->setCenter(39.90, 116.41);
    });

    auto *btnPanReset = new QPushButton(QStringLiteral("平移复位"), m_controlPanel);
    btnPanReset->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnPanReset);
    connect(btnPanReset, &QPushButton::clicked, this, [this]() {
        // 恢复初始位置阿尔及尔 (36.75°N, 3.05°E)
        m_mapContainer->setCenter(36.75, 3.05);
    });

    // ── 倾斜 API 演示 ──
    // setPitch(double pitch) - 设置地图倾斜角度
    //   pitch: 倾斜角度 [0, 60]，0=垂直俯视，60=最大倾斜
    //   需要样式支持 terrain 才能看到 3D 效果
    auto *btnTilt30 = new QPushButton(QStringLiteral("倾斜 30°"), m_controlPanel);
    btnTilt30->setStyleSheet(QStringLiteral("QPushButton { background-color: #009688; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnTilt30);
    connect(btnTilt30, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setPitch(30.0);
    });

    auto *btnTilt60 = new QPushButton(QStringLiteral("倾斜 60°"), m_controlPanel);
    btnTilt60->setStyleSheet(QStringLiteral("QPushButton { background-color: #009688; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnTilt60);
    connect(btnTilt60, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setPitch(60.0);
    });

    auto *btnTiltReset = new QPushButton(QStringLiteral("倾斜复位"), m_controlPanel);
    btnTiltReset->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnTiltReset);
    connect(btnTiltReset, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setPitch(0.0);
    });

    // ── 标注 API 演示 ──
    // setAnnotations(QVector<MapAnnotation>, QMap<QString, QImage>) - 批量添加标注
    //   每个标注包含：id, latitude, longitude, title, iconName
    //   icons 参数为图标名称到 QImage 的映射
    auto *btnAnnAdd = new QPushButton(QStringLiteral("添加标注"), m_controlPanel);
    btnAnnAdd->setStyleSheet(QStringLiteral("QPushButton { background-color: #F44336; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnAnnAdd);
    connect(btnAnnAdd, &QPushButton::clicked, this, [this]() {
        QMap<QString, QImage> icons;
        QImage icon(32, 32, QImage::Format_ARGB32);
        icon.fill(Qt::red);
        icons["marker"] = icon;

        QVector<MapAnnotation> anns;
        MapAnnotation a1;
        a1.id = "demo-ann-1";
        a1.latitude = 36.75;
        a1.longitude = 3.05;
        a1.title = "Demo Point";
        a1.iconName = "marker";
        anns.append(a1);

        // 添加标注到地图
        m_mapContainer->setAnnotations(anns, icons);
    });

    auto *btnAnnHide = new QPushButton(QStringLiteral("隐藏标注"), m_controlPanel);
    btnAnnHide->setStyleSheet(QStringLiteral("QPushButton { background-color: #F44336; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnAnnHide);
    connect(btnAnnHide, &QPushButton::clicked, this, [this]() {
        m_mapContainer->hideAllAnnotations();
    });

    auto *btnAnnShow = new QPushButton(QStringLiteral("显示标注"), m_controlPanel);
    btnAnnShow->setStyleSheet(QStringLiteral("QPushButton { background-color: #F44336; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnAnnShow);
    connect(btnAnnShow, &QPushButton::clicked, this, [this]() {
        m_mapContainer->showAllAnnotations();
    });

    auto *btnAnnClear = new QPushButton(QStringLiteral("清除标注"), m_controlPanel);
    btnAnnClear->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnAnnClear);
    connect(btnAnnClear, &QPushButton::clicked, this, [this]() {
        m_mapContainer->clearAnnotations();
    });

    // ── 线路 API 演示 ──
    // setRoutes(QVector<MapRouteSegment>) - 批量添加线路
    //   每条线路包含：id, routeId, coordinates, color, width, dashed
    auto *btnRouteAdd = new QPushButton(QStringLiteral("添加线路"), m_controlPanel);
    btnRouteAdd->setStyleSheet(QStringLiteral("QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnRouteAdd);
    connect(btnRouteAdd, &QPushButton::clicked, this, [this]() {
        QVector<MapRouteSegment> segs;
        MapRouteSegment seg;
        seg.id = "demo-route-1";
        seg.routeId = "route-A";
        seg.coordinates = {{36.75, 3.05}, {36.76, 3.06}, {36.77, 3.07}};
        seg.color = QColor(255, 0, 0);
        seg.width = 3.0;
        seg.dashed = false;
        segs.append(seg);

        m_mapContainer->setRoutes(segs);
    });

    auto *btnRouteHide = new QPushButton(QStringLiteral("隐藏线路"), m_controlPanel);
    btnRouteHide->setStyleSheet(QStringLiteral("QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnRouteHide);
    connect(btnRouteHide, &QPushButton::clicked, this, [this]() {
        m_mapContainer->hideAllRoutes();
    });

    auto *btnRouteShow = new QPushButton(QStringLiteral("显示线路"), m_controlPanel);
    btnRouteShow->setStyleSheet(QStringLiteral("QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnRouteShow);
    connect(btnRouteShow, &QPushButton::clicked, this, [this]() {
        m_mapContainer->showAllRoutes();
    });

    auto *btnRouteClear = new QPushButton(QStringLiteral("清除线路"), m_controlPanel);
    btnRouteClear->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnRouteClear);
    connect(btnRouteClear, &QPushButton::clicked, this, [this]() {
        m_mapContainer->clearRoutes();
    });

    // ── 位置 API 演示 ──
    // setLocation(double lat, double lon) - 设置位置指示器坐标
    // setLocationMode(LocationMode) - 设置模式：Fixed/Free
    // showLocation() / hideLocation() - 显示/隐藏位置指示器
    auto *btnLocShow = new QPushButton(QStringLiteral("显示位置"), m_controlPanel);
    btnLocShow->setStyleSheet(QStringLiteral("QPushButton { background-color: #3F51B5; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnLocShow);
    connect(btnLocShow, &QPushButton::clicked, this, [this]() {
        QImage icon(32, 32, QImage::Format_ARGB32);
        icon.fill(Qt::blue);
        m_mapContainer->setLocationIcon(icon);
        m_mapContainer->setLocation(36.75, 3.05);
        m_mapContainer->showLocation();
    });

    auto *btnLocHide = new QPushButton(QStringLiteral("隐藏位置"), m_controlPanel);
    btnLocHide->setStyleSheet(QStringLiteral("QPushButton { background-color: #3F51B5; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnLocHide);
    connect(btnLocHide, &QPushButton::clicked, this, [this]() {
        m_mapContainer->hideLocation();
    });

    auto *btnLocFree = new QPushButton(QStringLiteral("Free模式"), m_controlPanel);
    btnLocFree->setStyleSheet(QStringLiteral("QPushButton { background-color: #3F51B5; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnLocFree);
    connect(btnLocFree, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setLocationMode(LocationIndicatorManager::LocationMode::Free);
    });

    // ── Fixed 模式拖动控制演示 ──
    auto *btnFixedBlocked = new QPushButton(QStringLiteral("Fixed+不可拖动"), m_controlPanel);
    btnFixedBlocked->setStyleSheet(QStringLiteral("QPushButton { background-color: #FF5722; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnFixedBlocked);
    connect(btnFixedBlocked, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
        m_mapContainer->setCenterOffset(400);
        m_mapContainer->setFixedTouchPanEnabled(false);
    });

    auto *btnFixedAllowed = new QPushButton(QStringLiteral("Fixed+可拖动"), m_controlPanel);
    btnFixedAllowed->setStyleSheet(QStringLiteral("QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    m_controlPanel->layout()->addWidget(btnFixedAllowed);
    connect(btnFixedAllowed, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
        m_mapContainer->setCenterOffset(400);
        m_mapContainer->setFixedTouchPanEnabled(true);
        m_mapContainer->setFixedTouchResumeTimeout(3000);
    });

    // ── 综合复位按钮 ──
    // 一键恢复所有地图状态到初始值
    auto *btnResetAll = new QPushButton(QStringLiteral("全部复位"), m_controlPanel);
    btnResetAll->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #333333; color: white; font-size: %1px; padding: %2px; border: 1px solid #666; }"
    ).arg(btnFontSize).arg(btnPadding + 2));
    m_controlPanel->layout()->addWidget(btnResetAll);
    connect(btnResetAll, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setZoom(8.0);
        m_mapContainer->setBearing(0.0);
        m_mapContainer->setPitch(0.0);
        m_mapContainer->setCenter(36.75, 3.05);
        m_mapContainer->clearAnnotations();
        m_mapContainer->clearRoutes();
        m_mapContainer->hideLocation();
    });

    // ============================================================
    // TestRunner 自动化测试系统集成
    // ============================================================
    auto *testButton = new QPushButton(QStringLiteral("运行测试"), m_controlPanel);
    testButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }"
        "QPushButton:disabled { background-color: #666666; }"
    ).arg(btnFontSize + 1).arg(btnPadding + 2));
    m_controlPanel->layout()->addWidget(testButton);

    m_testLogView = new QTextBrowser(m_controlPanel);
    m_testLogView->setMaximumHeight(logViewMaxHeight);
    m_testLogView->setStyleSheet(QStringLiteral(
        "QTextBrowser { background-color: rgba(0,0,0,180); color: #00FF00; font-size: %1px; }"
    ).arg(logFontSize));
    m_testLogView->setPlaceholderText(QStringLiteral("点击\"运行测试\"开始"));
    m_controlPanel->layout()->addWidget(m_testLogView);

    // ============================================================
    // TestRunner 自动化测试系统集成
    // ============================================================
    // TestRunner 封装了完整的地图功能测试序列，通过 QTimer 实现异步非阻塞执行。
    // 测试覆盖范围：
    //   1. 地图基础操作：缩放(Z6/Z12/Z8)、旋转(0°/45°/90°)、平移(Algiers/上海/北京)、倾斜(0°/30°/60°)
    //   2. 标注功能：单点添加、批量添加、显示/隐藏、单点删除、全部清除
    //   3. 线路功能：单条添加、批量添加、显示/隐藏、单段删除、全部清除
    //   4. 位置指示器：设置坐标、显示/隐藏、Fixed/Free 模式切换、旋转、中心偏移
    //   5. 综合测试：标注与线路同时显示、图层独立切换
    //
    // 使用方式：
    //   - 点击绿色"运行测试"按钮启动
    //   - 测试结果实时显示在下方 QTextBrowser 中（带颜色区分 PASS/FAIL/STEP）
    //   - 测试完成后按钮恢复可点击状态，显示统计摘要
    //   - 在 Android 设备上可通过 adb logcat 查看 [TEST] 标签的日志输出
    //
    // 注意事项：
    //   - 测试依赖地图已就绪（mapReady 信号），请在地图加载完成后再启动
    //   - 每步之间有 1500ms 延迟，方便观察地图变化（可通过 setStepDelayMs 调整）
    //   - 测试期间地图控件仍可交互，测试在独立线程中异步执行
    // ============================================================
    m_testRunner = new TestRunner(m_mapContainer, this);

    // 点击"运行测试"按钮：禁用按钮防止重复启动，清空日志，开始测试序列
    connect(testButton, &QPushButton::clicked, this, [this, testButton]() {
        testButton->setEnabled(false);
        testButton->setText(QStringLiteral("测试中..."));
        m_testLogView->clear();
        m_testLogView->append(QStringLiteral("=== 测试开始 ==="));
        m_testRunner->startTests();
    });

    // 接收测试日志消息，按级别显示不同颜色：
    //   PASS - 绿色 (#00FF00)  |  FAIL - 红色 (#FF0000)  |  STEP - 黄色 (#FFFF00)  |  其他 - 白色
    connect(m_testRunner, &TestRunner::logMessage, this, [this](const QString &level, const QString &message) {
        QString color;
        if (level == QStringLiteral("PASS")) color = QStringLiteral("#00FF00");
        else if (level == QStringLiteral("FAIL")) color = QStringLiteral("#FF0000");
        else if (level == QStringLiteral("STEP")) color = QStringLiteral("#FFFF00");
        else color = QStringLiteral("#FFFFFF");
        m_testLogView->append(QStringLiteral("<font color=\"%1\">[%2] %3</font>").arg(color, level, message));
    });

    // 所有测试完成后：恢复按钮、显示统计摘要（通过数/总数/失败数/耗时）
    connect(m_testRunner, &TestRunner::allTestsFinished, this, [this, testButton](int total, int passed, int failed, int durationMs) {
        testButton->setEnabled(true);
        testButton->setText(QStringLiteral("运行测试"));
        QString color = (failed == 0) ? QStringLiteral("#00FF00") : QStringLiteral("#FF0000");
        m_testLogView->append(QStringLiteral("<font color=\"%1\">=== 完成: %2/%3 通过, %4 失败 (%5ms) ===</font>")
            .arg(color, QString::number(passed), QString::number(total), QString::number(failed), QString::number(durationMs)));
    });

    // ── 信号-槽连接 ─────────────────────────────────────────

    // 1. ControlPanel → Map (用户输入)
    //    用户通过控制面板的滑块调整地图参数
    connect(m_controlPanel, &ControlPanelWidget::zoomChanged,
            this, [this](double zoom) { m_mapContainer->map()->setZoom(zoom); });
    connect(m_controlPanel, &ControlPanelWidget::bearingChanged,
            this, [this](double bearing) { m_mapContainer->map()->setBearing(bearing); });
    connect(m_controlPanel, &ControlPanelWidget::tiltChanged,
            this, [this](double tilt) { m_mapContainer->map()->setPitch(tilt); });

    // 2. Map → ControlPanel (状态反馈)
    //    地图状态因用户手势/鼠标操作改变时，同步更新控制面板滑块
    connect(m_mapContainer, &MapContainer::zoomChanged,
            m_controlPanel, &ControlPanelWidget::setZoomValue);
    connect(m_mapContainer, &MapContainer::bearingChanged,
            m_controlPanel, &ControlPanelWidget::setBearingValue);
    connect(m_mapContainer, &MapContainer::tiltChanged,
            m_controlPanel, &ControlPanelWidget::setTiltValue);

    // 3. Map → ScaleBar (比例尺更新)
    //    地图缩放或中心位置变化时，重新计算并更新比例尺显示
    connect(m_mapContainer, &MapContainer::zoomChanged,
            this, [this](double zoom) {
        m_scaleBar->updateScale(m_mapContainer->map()->latitude(), zoom);
    });
    connect(m_mapContainer, &MapContainer::centerChanged,
            this, [this](double lat, double lon) {
        Q_UNUSED(lon);
        m_scaleBar->updateScale(lat, m_mapContainer->map()->zoom());
    });

    // 4. 触摸手势期间禁用非地图控件更新，减少 CPU 竞争
    connect(m_mapContainer, &MapContainer::touchBegin,
            this, [this]() { setGestureActive(true); });
    connect(m_mapContainer, &MapContainer::touchEnd,
            this, [this]() { setGestureActive(false); });

    // 5. 标注图层开关
    connect(m_annotationLayerToggle, &QCheckBox::toggled,
            this, [this](bool checked) {
        if (checked)
            m_mapContainer->showAllAnnotations();
        else
            m_mapContainer->hideAllAnnotations();
    });

    // 6. 线路图层开关
    connect(m_routeLayerToggle, &QCheckBox::toggled,
            this, [this](bool checked) {
        if (checked)
            m_mapContainer->showAllRoutes();
        else
            m_mapContainer->hideAllRoutes();
    });

    // 7. 位置图层开关
    connect(m_locationLayerToggle, &QCheckBox::toggled,
            this, [this](bool checked) {
        if (checked)
            m_mapContainer->showLocation();
        else
            m_mapContainer->hideLocation();
    });
}

/**
 * @brief 窗口尺寸变化事件处理
 * 窗口大小改变时重新定位比例尺，确保其始终位于地图左下角
 */
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    repositionScaleBar();
}

/**
 * @brief 将比例尺定位到 MapContainer 左下角
 *
 * 定位逻辑：
 * - x = 16: 距离左边缘 16px
 * - y = height - scaleBarHeight - 24: 距离底部 24px
 * 这样比例尺始终固定在地图可视区域的左下角
 */
void MainWindow::repositionScaleBar()
{
    m_scaleBar->move(16, m_mapContainer->height() - m_scaleBar->height() - 24);
}

void MainWindow::setGestureActive(bool active)
{
    // 在触摸手势期间禁用 ControlPanel 和 ScaleBar 的更新
    // 避免 mapChanged 信号触发大量 QWidget 重绘，减少 CPU 与 GL 线程竞争
    if (active) {
        m_controlPanel->setUpdatesEnabled(false);
        m_scaleBar->setUpdatesEnabled(false);
    } else {
        m_controlPanel->setUpdatesEnabled(true);
        m_scaleBar->setUpdatesEnabled(true);
        // 手势结束后强制更新一次，同步最终状态
        m_controlPanel->update();
        m_scaleBar->update();
    }
}
