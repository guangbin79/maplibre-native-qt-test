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
#include <QScrollArea>
#include <QScroller>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include "geojsonexporter.h"
#include "geojsonimporter.h"
#include <ttsplayer/TTSPlayer.h>
#ifdef IS_ANDROID
    #include <QJniObject>
    #include <QtCore/qcoreapplication_platform.h>
#endif

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
    , m_polygonLayerToggle(nullptr)
    , m_locationLayerToggle(nullptr)
    , m_testRunner(nullptr)
    , m_testLogView(nullptr)
    , m_ttsPlayer(nullptr)
    , m_ttsButton(nullptr)
{
    setWindowTitle(QStringLiteral("Map Viewer"));

    // ============================================================
    // 跨平台：窗口尺寸
    // 桌面默认 1200x800，Android 全屏（由系统控制）
    // ============================================================
#ifndef Q_OS_ANDROID
    resize(1200, 700);
#endif

    // ============================================================
    // 跨平台：配置参数
    // ============================================================
#ifdef Q_OS_ANDROID
    // Android：HXGISServer 在本地启动，使用 loopback 地址
    const QString serverHost = QStringLiteral("127.0.0.1");
    const QString serverUrl = QStringLiteral("http://%1:4943/styles/day/style.json?schema=hxmap").arg(serverHost);

    // Android：数据目录为外部存储 /storage/emulated/0/TitanNavi/map_data
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
    m_mapContainer->installEventFilter(this);

    // 启用 Fixed 模式下的触摸平移暂停功能：
    // 用户在 Fixed 模式下单指拖动地图时，自动暂停 Fixed 跟随（切换到 Free 显示），
    // 松手后经过超时时间自动恢复 Fixed 模式。
    m_mapContainer->setFixedTouchPanEnabled(false);
    m_mapContainer->setFixedTouchResumeTimeout(3000);

    // 控制面板 - 固定宽度，容纳 API 演示按钮 (stretch=0)
    m_controlPanel = new ControlPanelWidget(central);
    m_controlPanel->setFixedWidth(panelWidth);
    layout->addWidget(m_controlPanel, 0);

    // 创建滚动区域容纳所有按钮，防止窗口被强制拉伸
    auto *scrollArea = new QScrollArea(m_controlPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QStringLiteral("QScrollArea { border: none; background: transparent; }"));
    scrollArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
    QScroller::grabGesture(scrollArea->viewport(), QScroller::LeftMouseButtonGesture);
    auto *scrollWidget = new QWidget();
    auto *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setSpacing(2);
    scrollLayout->setContentsMargins(2, 2, 2, 2);
    scrollArea->setWidget(scrollWidget);
    m_controlPanel->layout()->addWidget(scrollArea);

    // 比例尺 - 作为 MapContainer 的子部件，定位在左下角
    m_scaleBar = new ScaleBarWidget(m_mapContainer);
    repositionScaleBar();

    // 标注图层开关 - 在滚动区域中添加复选框
    m_annotationLayerToggle = new QCheckBox(QStringLiteral("标注"), m_controlPanel);
    m_annotationLayerToggle->setChecked(true);
    m_annotationLayerToggle->setStyleSheet(QStringLiteral("color: white; font-size: 11px;"));
    scrollLayout->addWidget(m_annotationLayerToggle);

    m_routeLayerToggle = new QCheckBox(QStringLiteral("线路"), m_controlPanel);
    m_routeLayerToggle->setChecked(true);
    m_routeLayerToggle->setStyleSheet(QStringLiteral("color: white; font-size: 11px;"));
    scrollLayout->addWidget(m_routeLayerToggle);

    m_polygonLayerToggle = new QCheckBox(QStringLiteral("多边形"), m_controlPanel);
    m_polygonLayerToggle->setChecked(true);
    m_polygonLayerToggle->setStyleSheet(QStringLiteral("color: white; font-size: 11px;"));
    scrollLayout->addWidget(m_polygonLayerToggle);

    m_locationLayerToggle = new QCheckBox(QStringLiteral("位置"), m_controlPanel);
    m_locationLayerToggle->setChecked(false);
    m_locationLayerToggle->setStyleSheet(QStringLiteral("color: white; font-size: 11px;"));
    scrollLayout->addWidget(m_locationLayerToggle);

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
    scrollLayout->addWidget(btnZoomOut);
    connect(btnZoomOut, &QPushButton::clicked, this, [this]() {
        // 缩小到 Z6，观察国家级别视图
        m_mapContainer->setZoom(6.0);
    });

    auto *btnZoomIn = new QPushButton(QStringLiteral("放大 (Z12)"), m_controlPanel);
    btnZoomIn->setStyleSheet(QStringLiteral("QPushButton { background-color: #2196F3; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnZoomIn);
    connect(btnZoomIn, &QPushButton::clicked, this, [this]() {
        // 放大到 Z12，观察街道级别细节
        m_mapContainer->setZoom(12.0);
    });

    auto *btnZoomReset = new QPushButton(QStringLiteral("缩放复位"), m_controlPanel);
    btnZoomReset->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnZoomReset);
    connect(btnZoomReset, &QPushButton::clicked, this, [this]() {
        // 恢复默认缩放级别 Z8
        m_mapContainer->setZoom(8.0);
    });

    // ── 旋转 API 演示 ──
    // setBearing(double bearing) - 设置地图旋转角度（方位角）
    //   bearing: 顺时针旋转角度，单位度。0=正北朝上
    auto *btnRotate45 = new QPushButton(QStringLiteral("旋转 45°"), m_controlPanel);
    btnRotate45->setStyleSheet(QStringLiteral("QPushButton { background-color: #FF9800; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnRotate45);
    connect(btnRotate45, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setBearing(45.0);
    });

    auto *btnRotate90 = new QPushButton(QStringLiteral("旋转 90°"), m_controlPanel);
    btnRotate90->setStyleSheet(QStringLiteral("QPushButton { background-color: #FF9800; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnRotate90);
    connect(btnRotate90, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setBearing(90.0);
    });

    auto *btnRotateReset = new QPushButton(QStringLiteral("旋转复位"), m_controlPanel);
    btnRotateReset->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnRotateReset);
    connect(btnRotateReset, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setBearing(0.0);
    });

    // ── 平移 API 演示 ──
    // setCenter(double lat, double lon) - 设置地图中心坐标
    //   lat: 纬度 [-90, 90], lon: 经度 [-180, 180]
    //   注意：坐标顺序是 (lat, lon)，与 GeoJSON (lon, lat) 不同
    auto *btnPanShanghai = new QPushButton(QStringLiteral("平移→上海"), m_controlPanel);
    btnPanShanghai->setStyleSheet(QStringLiteral("QPushButton { background-color: #9C27B0; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnPanShanghai);
    connect(btnPanShanghai, &QPushButton::clicked, this, [this]() {
        // 平移到上海 (31.23°N, 121.47°E)
        m_mapContainer->setCenter(31.23, 121.47);
    });

    auto *btnPanBeijing = new QPushButton(QStringLiteral("平移→北京"), m_controlPanel);
    btnPanBeijing->setStyleSheet(QStringLiteral("QPushButton { background-color: #9C27B0; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnPanBeijing);
    connect(btnPanBeijing, &QPushButton::clicked, this, [this]() {
        // 平移到北京 (39.90°N, 116.41°E)
        m_mapContainer->setCenter(39.90, 116.41);
    });

    auto *btnPanReset = new QPushButton(QStringLiteral("平移复位"), m_controlPanel);
    btnPanReset->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnPanReset);
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
    scrollLayout->addWidget(btnTilt30);
    connect(btnTilt30, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setPitch(30.0);
    });

    auto *btnTilt60 = new QPushButton(QStringLiteral("倾斜 60°"), m_controlPanel);
    btnTilt60->setStyleSheet(QStringLiteral("QPushButton { background-color: #009688; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnTilt60);
    connect(btnTilt60, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setPitch(60.0);
    });

    auto *btnTiltReset = new QPushButton(QStringLiteral("倾斜复位"), m_controlPanel);
    btnTiltReset->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnTiltReset);
    connect(btnTiltReset, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setPitch(0.0);
    });

    // ── 标注 API 演示 ──
    // 标注 API 分两步调用：
    //   1. registerAnnotationIcons(QMap<QString, QImage>) - 先注册图标
    //   2. setAnnotations(QVector<MapAnnotation>) - 再添加标注数据
    //   每个标注包含：id, latitude, longitude, title, iconName
    auto *btnAnnAdd = new QPushButton(QStringLiteral("添加标注"), m_controlPanel);
    btnAnnAdd->setStyleSheet(QStringLiteral("QPushButton { background-color: #F44336; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnAnnAdd);
    connect(btnAnnAdd, &QPushButton::clicked, this, [this]() {
        // 注册两种不同颜色的图标
        QMap<QString, QImage> icons;
        QImage markerIcon(32, 32, QImage::Format_ARGB32);
        markerIcon.fill(Qt::red);
        icons["marker"] = markerIcon;

        QImage flagIcon(32, 32, QImage::Format_ARGB32);
        flagIcon.fill(Qt::blue);
        icons["flag"] = flagIcon;

        QVector<MapAnnotation> anns;

        // 标注 1: 北京天安门 (红色 marker)
        MapAnnotation a1;
        a1.id = "demo-ann-1";
        a1.latitude = 39.9042;
        a1.longitude = 116.4074;
        a1.title = QStringLiteral("北京天安门");
        a1.iconName = "marker";
        anns.append(a1);

        // 标注 2: 上海外滩 (蓝色 flag)
        MapAnnotation a2;
        a2.id = "demo-ann-2";
        a2.latitude = 31.2304;
        a2.longitude = 121.4737;
        a2.title = QStringLiteral("上海外滩");
        a2.iconName = "flag";
        anns.append(a2);

        // 标注 3: 广州塔 (红色 marker)
        MapAnnotation a3;
        a3.id = "demo-ann-3";
        a3.latitude = 23.1291;
        a3.longitude = 113.2644;
        a3.title = QStringLiteral("广州塔");
        a3.iconName = "marker";
        anns.append(a3);

        m_mapContainer->registerAnnotationIcons(icons);
        m_mapContainer->setAnnotations(anns);

        // 自动缩放到包含所有标注
        m_mapContainer->setCenter(31.75, 116.70);
        m_mapContainer->setZoom(5.0);
    });

    auto *btnAnnHide = new QPushButton(QStringLiteral("隐藏标注"), m_controlPanel);
    btnAnnHide->setStyleSheet(QStringLiteral("QPushButton { background-color: #F44336; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnAnnHide);
    connect(btnAnnHide, &QPushButton::clicked, this, [this]() {
        m_mapContainer->hideAllAnnotations();
    });

    auto *btnAnnShow = new QPushButton(QStringLiteral("显示标注"), m_controlPanel);
    btnAnnShow->setStyleSheet(QStringLiteral("QPushButton { background-color: #F44336; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnAnnShow);
    connect(btnAnnShow, &QPushButton::clicked, this, [this]() {
        m_mapContainer->showAllAnnotations();
    });

    auto *btnAnnClear = new QPushButton(QStringLiteral("清除标注"), m_controlPanel);
    btnAnnClear->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnAnnClear);
    connect(btnAnnClear, &QPushButton::clicked, this, [this]() {
        m_mapContainer->clearAnnotations();
    });

    // ── 纯文字标注演示 ──
    // 不带图标的纯文字标注，展示文字标注效果
    auto *btnTextAnn = new QPushButton(QStringLiteral("文字标注"), m_controlPanel);
    btnTextAnn->setStyleSheet(QStringLiteral("QPushButton { background-color: #E91E63; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnTextAnn);
    connect(btnTextAnn, &QPushButton::clicked, this, [this]() {
        QVector<MapAnnotation> anns;

        MapAnnotation a1;
        a1.id = "text-ann-en";
        a1.latitude = 36.7538;
        a1.longitude = 3.0588;
        a1.title = QStringLiteral("Algiers");
        a1.iconName = "";
        anns.append(a1);

        MapAnnotation a2;
        a2.id = "text-ann-fr";
        a2.latitude = 36.76;
        a2.longitude = 3.07;
        a2.title = QStringLiteral("Bonjour");
        a2.iconName = "";
        anns.append(a2);

        MapAnnotation a3;
        a3.id = "text-ann-ar";
        a3.latitude = 36.745;
        a3.longitude = 3.05;
        a3.title = QStringLiteral("مرحبا");
        a3.iconName = "";
        anns.append(a3);

        MapAnnotation a4;
        a4.id = "text-ann-fa";
        a4.latitude = 36.735;
        a4.longitude = 3.065;
        a4.title = QStringLiteral("سلام");
        a4.iconName = "";
        anns.append(a4);

        MapAnnotation a5;
        a5.id = "text-ann-ru";
        a5.latitude = 36.755;
        a5.longitude = 3.075;
        a5.title = QStringLiteral("Привет");
        a5.iconName = "";
        anns.append(a5);

        MapAnnotation a6;
        a6.id = "text-ann-zh";
        a6.latitude = 36.765;
        a6.longitude = 3.055;
        a6.title = QStringLiteral("你好");
        a6.iconName = "";
        anns.append(a6);

        m_mapContainer->setAnnotations(anns);

        // 缩放到标注区域
        m_mapContainer->setCenter(36.756, 3.064);
        m_mapContainer->setZoom(14.0);
    });

    // ── 线路 API 演示 ──
    // setRoutes(QVector<MapRouteSegment>) - 批量添加线路
    //   每条线路包含：id, routeId, coordinates, color, width, dashed
    auto *btnRouteAdd = new QPushButton(QStringLiteral("添加线路"), m_controlPanel);
    btnRouteAdd->setStyleSheet(QStringLiteral("QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnRouteAdd);
    connect(btnRouteAdd, &QPushButton::clicked, this, [this]() {
        QVector<MapRouteSegment> segs;

        // 红色实线线路 (route-A)
        MapRouteSegment seg1;
        seg1.id = "demo-route-1";
        seg1.routeId = "route-A";
        seg1.coordinates = {{36.75, 3.05}, {36.76, 3.06}, {36.77, 3.07}};
        seg1.color = QColor(255, 0, 0);
        seg1.width = 4.0;
        seg1.dashed = false;
        segs.append(seg1);

        // 蓝色虚线线路 (route-B)
        MapRouteSegment seg2;
        seg2.id = "demo-route-2";
        seg2.routeId = "route-B";
        seg2.coordinates = {{36.74, 3.08}, {36.75, 3.09}, {36.76, 3.10}};
        seg2.color = QColor(0, 100, 255);
        seg2.width = 3.0;
        seg2.dashed = true;
        segs.append(seg2);

        // 绿色实线线路 (route-C)
        MapRouteSegment seg3;
        seg3.id = "demo-route-3";
        seg3.routeId = "route-C";
        seg3.coordinates = {{36.78, 3.03}, {36.78, 3.05}, {36.78, 3.07}};
        seg3.color = QColor(0, 200, 0);
        seg3.width = 5.0;
        seg3.dashed = false;
        segs.append(seg3);

        m_mapContainer->setRoutes(segs);
    });

    auto *btnRouteHide = new QPushButton(QStringLiteral("隐藏线路"), m_controlPanel);
    btnRouteHide->setStyleSheet(QStringLiteral("QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnRouteHide);
    connect(btnRouteHide, &QPushButton::clicked, this, [this]() {
        m_mapContainer->hideAllRoutes();
    });

    auto *btnRouteShow = new QPushButton(QStringLiteral("显示线路"), m_controlPanel);
    btnRouteShow->setStyleSheet(QStringLiteral("QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnRouteShow);
    connect(btnRouteShow, &QPushButton::clicked, this, [this]() {
        m_mapContainer->showAllRoutes();
    });

    auto *btnRouteClear = new QPushButton(QStringLiteral("清除线路"), m_controlPanel);
    btnRouteClear->setStyleSheet(QStringLiteral("QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnRouteClear);
    connect(btnRouteClear, &QPushButton::clicked, this, [this]() {
        m_mapContainer->clearRoutes();
    });

    // focusOnRoute(routeId, durationMs) - 聚焦到指定线路
    //   routeId: 线路ID（对应 MapRouteSegment::routeId）
    //   durationMs: 动画时长（毫秒），-1 使用默认值
    //   返回: true 成功聚焦，false 线路不存在或地图未就绪
    auto *btnRouteFocus = new QPushButton(QStringLiteral("聚焦线路"), m_controlPanel);
    btnRouteFocus->setStyleSheet(QStringLiteral("QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnRouteFocus);
    connect(btnRouteFocus, &QPushButton::clicked, this, [this]() {
        // 先确保有线路数据
        QVector<MapRouteSegment> segs;
        MapRouteSegment seg;
        seg.id = "focus-test-1";
        seg.routeId = "route-A";
        seg.coordinates = {{36.75, 3.05}, {36.76, 3.06}, {36.77, 3.07}};
        seg.color = QColor(255, 0, 0);
        seg.width = 3.0;
        seg.dashed = false;
        segs.append(seg);

        m_mapContainer->setRoutes(segs);
        // 聚焦到 route-A，地图自动缩放平移到包含该线路的区域
        bool ok = m_mapContainer->focusOnRoute("route-A");
        if (!ok) {
            qDebug() << "focusOnRoute failed: route not found or map not ready";
        }
    });

    // ── 多边形 API 演示 ──
    // setPolygons(QVector<MapPolygon>) - 批量添加多边形
    //   每个多边形包含：id, polygonId, coordinates, fillEnabled, fillColor,
    //                  fillOpacity, strokeColor, strokeWidth, strokeDashed, title
    auto *btnPolygonAdd = new QPushButton(QStringLiteral("添加多边形"), m_controlPanel);
    btnPolygonAdd->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }"
    ).arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnPolygonAdd);
    connect(btnPolygonAdd, &QPushButton::clicked, this, [this]() {
        QVector<MapPolygon> polys;

        // 红色半透明填充实线矩形
        MapPolygon poly1;
        poly1.id = "demo-poly-1";
        poly1.polygonId = "polygon-demo-A";
        poly1.coordinates = {{36.74, 3.04}, {36.76, 3.04}, {36.76, 3.06}, {36.74, 3.06}};
        poly1.fillEnabled = true;
        poly1.fillColor = QColor(255, 0, 0);
        poly1.fillOpacity = 0.3;
        poly1.strokeColor = QColor(255, 0, 0);
        poly1.strokeWidth = 2.0;
        poly1.strokeDashed = false;
        poly1.title = QStringLiteral("红色区域");
        polys.append(poly1);

        // 蓝色虚线边框三角形
        MapPolygon poly2;
        poly2.id = "demo-poly-2";
        poly2.polygonId = "polygon-demo-B";
        poly2.coordinates = {{36.77, 3.05}, {36.79, 3.07}, {36.78, 3.03}};
        poly2.fillEnabled = true;
        poly2.fillColor = QColor(0, 0, 255);
        poly2.fillOpacity = 0.2;
        poly2.strokeColor = QColor(0, 0, 255);
        poly2.strokeWidth = 2.0;
        poly2.strokeDashed = true;
        poly2.title = QStringLiteral("蓝色三角");
        polys.append(poly2);

        // 绿色五边形 - 无填充、粗实线边框
        MapPolygon poly3;
        poly3.id = "demo-poly-3";
        poly3.polygonId = "polygon-demo-C";
        poly3.coordinates = {{36.80, 3.05}, {36.81, 3.06}, {36.805, 3.08}, {36.795, 3.08}, {36.79, 3.06}};
        poly3.fillEnabled = false;
        poly3.fillColor = QColor(0, 0, 0);
        poly3.fillOpacity = 0.0;
        poly3.strokeColor = QColor(0, 200, 0);
        poly3.strokeWidth = 4.0;
        poly3.strokeDashed = false;
        poly3.title = QStringLiteral("绿色五边形");
        polys.append(poly3);

        m_mapContainer->setPolygons(polys);
    });

    auto *btnPolygonFocus = new QPushButton(QStringLiteral("聚焦多边形"), m_controlPanel);
    btnPolygonFocus->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }"
    ).arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnPolygonFocus);
    connect(btnPolygonFocus, &QPushButton::clicked, this, [this]() {
        QVector<MapPolygon> polys;
        MapPolygon poly;
        poly.id = "focus-test-poly";
        poly.polygonId = "polygon-demo-A";
        poly.coordinates = {{36.74, 3.04}, {36.76, 3.04}, {36.76, 3.06}, {36.74, 3.06}};
        poly.fillEnabled = true;
        poly.fillColor = QColor(255, 0, 0);
        poly.fillOpacity = 0.3;
        poly.strokeColor = QColor(255, 0, 0);
        poly.strokeWidth = 2.0;
        m_mapContainer->setPolygons({poly});
        bool ok = m_mapContainer->focusOnPolygon("polygon-demo-A");
        if (!ok) {
            qDebug() << "focusOnPolygon failed";
        }
    });

    auto *btnPolygonClear = new QPushButton(QStringLiteral("清除多边形"), m_controlPanel);
    btnPolygonClear->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #757575; color: white; font-size: %1px; padding: %2px; }"
    ).arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnPolygonClear);
    connect(btnPolygonClear, &QPushButton::clicked, this, [this]() {
        m_mapContainer->clearPolygons();
    });

    // ===== 导出/导入 GeoJSON =====
    auto *btnExportGeoJson = new QPushButton(QStringLiteral("导出GeoJSON"), m_controlPanel);
    btnExportGeoJson->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #2196F3; color: white; padding: 6px; border-radius: 3px; font-size: 11px; }"
        "QPushButton:hover { background-color: #1976D2; }"));
    scrollLayout->addWidget(btnExportGeoJson);
    connect(btnExportGeoJson, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择导出目录"));
        if (dir.isEmpty()) return;

        QDir().mkpath(dir);

        // Export annotations
        QByteArray annData = GeoJsonExporter::buildAnnotations(m_mapContainer->annotations());
        QFile annFile(dir + "/annotations.geojson");
        if (annFile.open(QIODevice::WriteOnly)) {
            annFile.write(annData);
            annFile.close();
        }

        // Export routes
        QByteArray routeData = GeoJsonExporter::buildRoutes(m_mapContainer->segments());
        QFile routeFile(dir + "/routes.geojson");
        if (routeFile.open(QIODevice::WriteOnly)) {
            routeFile.write(routeData);
            routeFile.close();
        }

        // Export polygons
        QByteArray polyData = GeoJsonExporter::buildPolygons(m_mapContainer->polygons());
        QFile polyFile(dir + "/polygons.geojson");
        if (polyFile.open(QIODevice::WriteOnly)) {
            polyFile.write(polyData);
            polyFile.close();
        }

        qDebug() << "Exported to" << dir;
    });

    auto *btnImportGeoJson = new QPushButton(QStringLiteral("导入GeoJSON"), m_controlPanel);
    btnImportGeoJson->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #FF9800; color: white; padding: 6px; border-radius: 3px; font-size: 11px; }"
        "QPushButton:hover { background-color: #F57C00; }"));
    scrollLayout->addWidget(btnImportGeoJson);
    connect(btnImportGeoJson, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择导入目录"));
        if (dir.isEmpty()) return;

        // Import annotations
        QFile annFile(dir + "/annotations.geojson");
        if (annFile.exists() && annFile.open(QIODevice::ReadOnly)) {
            bool ok = false;
            auto anns = GeoJsonImporter::parseAnnotations(annFile.readAll(), &ok);
            annFile.close();
            if (ok) {
                m_mapContainer->clearAnnotations();
                m_mapContainer->addAnnotations(anns);
            }
        }

        // Import routes
        QFile routeFile(dir + "/routes.geojson");
        if (routeFile.exists() && routeFile.open(QIODevice::ReadOnly)) {
            bool ok = false;
            auto segs = GeoJsonImporter::parseRoutes(routeFile.readAll(), &ok);
            routeFile.close();
            if (ok) {
                m_mapContainer->clearRoutes();
                m_mapContainer->addRouteSegments(segs);
            }
        }

        // Import polygons
        QFile polyFile(dir + "/polygons.geojson");
        if (polyFile.exists() && polyFile.open(QIODevice::ReadOnly)) {
            bool ok = false;
            auto polys = GeoJsonImporter::parsePolygons(polyFile.readAll(), &ok);
            polyFile.close();
            if (ok) {
                m_mapContainer->clearPolygons();
                m_mapContainer->addPolygons(polys);
            }
        }

        qDebug() << "Imported from" << dir;
    });

    // ── 重置全部数据按钮 ──
    auto *btnResetData = new QPushButton(QStringLiteral("重置全部数据"), m_controlPanel);
    btnResetData->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #757575; color: white; padding: 6px; border-radius: 3px; font-size: 11px; }"
        "QPushButton:hover { background-color: #616161; }"));
    scrollLayout->addWidget(btnResetData);
    connect(btnResetData, &QPushButton::clicked, this, [this]() {
        m_mapContainer->clearAnnotations();
        m_mapContainer->clearRoutes();
        m_mapContainer->clearPolygons();
        m_mapContainer->hideLocation();
    });

    // ── 导出示例按钮 ──
    auto *btnExportDemo = new QPushButton(QStringLiteral("导出示例"), m_controlPanel);
    btnExportDemo->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #2196F3; color: white; padding: 6px; border-radius: 3px; font-size: 11px; }"
        "QPushButton:hover { background-color: #1976D2; }"));
    scrollLayout->addWidget(btnExportDemo);
    connect(btnExportDemo, &QPushButton::clicked, this, [this]() {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/mapviewer_export_demo";
        QDir().mkpath(dir);

        // 创建示例标注数据并导出
        QVector<MapAnnotation> demoAnns;
        MapAnnotation da;
        da.id = "export-demo-1";
        da.latitude = 36.75;
        da.longitude = 3.05;
        da.title = QStringLiteral("导出示例点");
        da.iconName = "marker";
        demoAnns.append(da);

        QByteArray annData = GeoJsonExporter::buildAnnotations(demoAnns);
        QFile annFile(dir + "/annotations.geojson");
        if (annFile.open(QIODevice::WriteOnly)) {
            annFile.write(annData);
            annFile.close();
        }

        // 创建示例线路数据并导出
        QVector<MapRouteSegment> demoSegs;
        MapRouteSegment ds;
        ds.id = "export-demo-route-1";
        ds.routeId = "export-route-A";
        ds.coordinates = {{36.75, 3.05}, {36.76, 3.06}};
        ds.color = QColor(255, 0, 0);
        ds.width = 3.0;
        ds.dashed = false;
        demoSegs.append(ds);

        QByteArray routeData = GeoJsonExporter::buildRoutes(demoSegs);
        QFile routeFile(dir + "/routes.geojson");
        if (routeFile.open(QIODevice::WriteOnly)) {
            routeFile.write(routeData);
            routeFile.close();
        }

        // 创建示例多边形数据并导出
        QVector<MapPolygon> demoPolys;
        MapPolygon dp;
        dp.id = "export-demo-poly-1";
        dp.polygonId = "export-polygon-A";
        dp.coordinates = {{36.74, 3.04}, {36.76, 3.04}, {36.76, 3.06}};
        dp.fillEnabled = true;
        dp.fillColor = QColor(0, 200, 0);
        dp.fillOpacity = 0.3;
        dp.strokeColor = QColor(0, 200, 0);
        dp.strokeWidth = 2.0;
        dp.strokeDashed = false;
        demoPolys.append(dp);

        QByteArray polyData = GeoJsonExporter::buildPolygons(demoPolys);
        QFile polyFile(dir + "/polygons.geojson");
        if (polyFile.open(QIODevice::WriteOnly)) {
            polyFile.write(polyData);
            polyFile.close();
        }

        qDebug() << "Demo exported to" << dir;
    });

    // ── 导入示例按钮 ──
    auto *btnImportDemo = new QPushButton(QStringLiteral("导入示例"), m_controlPanel);
    btnImportDemo->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #FF9800; color: white; padding: 6px; border-radius: 3px; font-size: 11px; }"
        "QPushButton:hover { background-color: #F57C00; }"));
    scrollLayout->addWidget(btnImportDemo);
    connect(btnImportDemo, &QPushButton::clicked, this, [this]() {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/mapviewer_import_demo";
        QDir().mkpath(dir);

        // 写入示例 GeoJSON 标注文件
        QFile annFile(dir + "/annotations.geojson");
        if (annFile.open(QIODevice::WriteOnly)) {
            annFile.write(R"({"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[3.05,36.75]},"properties":{"id":"import-demo-1","title":"导入示例点","iconName":"marker"}}]})");
            annFile.close();
        }

        // 写入示例 GeoJSON 线路文件
        QFile routeFile(dir + "/routes.geojson");
        if (routeFile.open(QIODevice::WriteOnly)) {
            routeFile.write(R"({"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"LineString","coordinates":[[3.05,36.75],[3.06,36.76],[3.07,36.77]]},"properties":{"id":"import-route-1","routeId":"import-route-A","color":"#FF0000","width":3.0,"dashed":false}}]})");
            routeFile.close();
        }

        // 写入示例 GeoJSON 多边形文件
        QFile polyFile(dir + "/polygons.geojson");
        if (polyFile.open(QIODevice::WriteOnly)) {
            polyFile.write(R"({"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Polygon","coordinates":[[[3.04,36.74],[3.04,36.76],[3.06,36.76],[3.06,36.74],[3.04,36.74]]]},"properties":{"id":"import-poly-1","polygonId":"import-polygon-A","fillEnabled":true,"fillColor":"#00FF00","fillOpacity":0.3,"strokeColor":"#00FF00","strokeWidth":2.0,"strokeDashed":false,"title":"导入示例区域"}}]})");
            polyFile.close();
        }

        // 导入标注
        if (annFile.exists() && annFile.open(QIODevice::ReadOnly)) {
            bool ok = false;
            auto anns = GeoJsonImporter::parseAnnotations(annFile.readAll(), &ok);
            annFile.close();
            if (ok) {
                m_mapContainer->clearAnnotations();
                m_mapContainer->addAnnotations(anns);
            }
        }

        // 导入线路
        if (routeFile.exists() && routeFile.open(QIODevice::ReadOnly)) {
            bool ok = false;
            auto segs = GeoJsonImporter::parseRoutes(routeFile.readAll(), &ok);
            routeFile.close();
            if (ok) {
                m_mapContainer->clearRoutes();
                m_mapContainer->addRouteSegments(segs);
            }
        }

        // 导入多边形
        if (polyFile.exists() && polyFile.open(QIODevice::ReadOnly)) {
            bool ok = false;
            auto polys = GeoJsonImporter::parsePolygons(polyFile.readAll(), &ok);
            polyFile.close();
            if (ok) {
                m_mapContainer->clearPolygons();
                m_mapContainer->addPolygons(polys);
            }
        }

        qDebug() << "Demo imported from" << dir;
    });

    // ── 位置 API 演示 ──
    // setLocation(double lat, double lon) - 设置位置指示器坐标
    // setLocationMode(LocationMode) - 设置模式：Fixed/Free
    // showLocation() / hideLocation() - 显示/隐藏位置指示器
    auto *btnLocShow = new QPushButton(QStringLiteral("显示位置"), m_controlPanel);
    btnLocShow->setStyleSheet(QStringLiteral("QPushButton { background-color: #3F51B5; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnLocShow);
    connect(btnLocShow, &QPushButton::clicked, this, [this]() {
        QImage icon(32, 32, QImage::Format_ARGB32);
        icon.fill(Qt::blue);
        m_mapContainer->setLocationIcon(icon);
        m_mapContainer->setLocation(36.75, 3.05);
        m_mapContainer->showLocation();
    });

    auto *btnLocHide = new QPushButton(QStringLiteral("隐藏位置"), m_controlPanel);
    btnLocHide->setStyleSheet(QStringLiteral("QPushButton { background-color: #3F51B5; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnLocHide);
    connect(btnLocHide, &QPushButton::clicked, this, [this]() {
        m_mapContainer->hideLocation();
    });

    auto *btnLocFree = new QPushButton(QStringLiteral("Free模式"), m_controlPanel);
    btnLocFree->setStyleSheet(QStringLiteral("QPushButton { background-color: #3F51B5; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnLocFree);
    connect(btnLocFree, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setLocationMode(LocationIndicatorManager::LocationMode::Free);
    });

    // ── Fixed 模式拖动控制演示 ──
    auto *btnFixedBlocked = new QPushButton(QStringLiteral("Fixed+不可拖动"), m_controlPanel);
    btnFixedBlocked->setStyleSheet(QStringLiteral("QPushButton { background-color: #FF5722; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnFixedBlocked);
    connect(btnFixedBlocked, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setLocationMode(LocationIndicatorManager::LocationMode::Fixed);
        m_mapContainer->setCenterOffset(400);
        m_mapContainer->setFixedTouchPanEnabled(false);
    });

    auto *btnFixedAllowed = new QPushButton(QStringLiteral("Fixed+可拖动"), m_controlPanel);
    btnFixedAllowed->setStyleSheet(QStringLiteral("QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }").arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(btnFixedAllowed);
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
    scrollLayout->addWidget(btnResetAll);
    connect(btnResetAll, &QPushButton::clicked, this, [this]() {
        m_mapContainer->setZoom(8.0);
        m_mapContainer->setBearing(0.0);
        m_mapContainer->setPitch(0.0);
        m_mapContainer->setCenter(36.75, 3.05);
        m_mapContainer->clearAnnotations();
        m_mapContainer->clearRoutes();
        m_mapContainer->clearPolygons();
        m_mapContainer->hideLocation();
    });

    // ============================================================
    // TTS 法语播放按钮
    // ============================================================
    m_ttsButton = new QPushButton(QStringLiteral("TTS 加载中..."), m_controlPanel);
    m_ttsButton->setEnabled(false);
    m_ttsButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #E91E63; color: white; font-size: %1px; padding: %2px; }"
        "QPushButton:disabled { background-color: #666666; }"
    ).arg(btnFontSize).arg(btnPadding));
    scrollLayout->addWidget(m_ttsButton);

    m_ttsPlayer = new TTSPlayer(this);

    // 连接 TTSPlayer 信号
    connect(m_ttsPlayer, &TTSPlayer::readyChanged, this, [this](bool ready) {
        if (ready) {
            m_ttsButton->setEnabled(true);
            m_ttsButton->setText(QStringLiteral("播放法语"));
            qDebug() << "TTSPlayer initialized successfully";
        } else {
            m_ttsButton->setEnabled(false);
            m_ttsButton->setText(QStringLiteral("TTS 未就绪"));
        }
    });

    connect(m_ttsPlayer, &TTSPlayer::playingChanged, this, [this](bool playing) {
        if (playing) {
            m_ttsButton->setEnabled(false);
            m_ttsButton->setText(QStringLiteral("播放中..."));
        } else {
            m_ttsButton->setEnabled(true);
            m_ttsButton->setText(QStringLiteral("播放法语"));
        }
    });

    connect(m_ttsPlayer, &TTSPlayer::errorOccurred, this, [this](const QString &errorMsg) {
        qWarning() << "TTSPlayer error:" << errorMsg;
        m_ttsButton->setEnabled(false);
        m_ttsButton->setText(QStringLiteral("TTS 错误"));
    });

    // 点击播放法语文本
    connect(m_ttsButton, &QPushButton::clicked, this, [this]() {
        m_ttsPlayer->play(QStringLiteral("Bonjour, ceci est un test de synthèse vocale française."));
    });

    // 初始化 TTSPlayer - 平台特定的模型路径
    {
        QString ttsModelPath;
#ifdef IS_ANDROID
        ttsModelPath = QStringLiteral("/storage/emulated/0/TitanNavi/tts_models/fr_FR-siwis-low");
        if (!m_ttsPlayer->initialize(ttsModelPath)) {
            qWarning() << "TTSPlayer initialization failed for path:" << ttsModelPath;
            m_ttsButton->setText(QStringLiteral("TTS 初始化失败"));
        }
#else
        // Linux: 直接使用项目目录下的模型
        ttsModelPath = QStringLiteral("%1/../../ttsplayer-1.0.0/models/fr_FR-siwis-low")
            .arg(QCoreApplication::applicationDirPath());
        if (!m_ttsPlayer->initialize(ttsModelPath)) {
            qWarning() << "TTSPlayer initialization failed for path:" << ttsModelPath;
            m_ttsButton->setText(QStringLiteral("TTS 初始化失败"));
        }
#endif
    }

    // ============================================================
    // TestRunner 自动化测试系统集成
    // ============================================================
    auto *testButton = new QPushButton(QStringLiteral("运行测试"), m_controlPanel);
    testButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #4CAF50; color: white; font-size: %1px; padding: %2px; }"
        "QPushButton:disabled { background-color: #666666; }"
    ).arg(btnFontSize + 1).arg(btnPadding + 2));
    scrollLayout->addWidget(testButton);

    m_testLogView = new QTextBrowser(m_controlPanel);
    m_testLogView->setMaximumHeight(logViewMaxHeight);
    m_testLogView->setStyleSheet(QStringLiteral(
        "QTextBrowser { background-color: rgba(0,0,0,180); color: #00FF00; font-size: %1px; }"
    ).arg(logFontSize));
    m_testLogView->setPlaceholderText(QStringLiteral("点击\"运行测试\"开始"));
    scrollLayout->addWidget(m_testLogView);

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

    // 8. 多边形图层开关
    connect(m_polygonLayerToggle, &QCheckBox::toggled,
            this, [this](bool checked) {
        if (checked)
            m_mapContainer->showAllPolygons();
        else
            m_mapContainer->hideAllPolygons();
    });

    // 7. 位置图层开关
    connect(m_locationLayerToggle, &QCheckBox::toggled,
            this, [this](bool checked) {
        if (checked)
            m_mapContainer->showLocation();
        else
            m_mapContainer->hideLocation();
    });

    connect(m_mapContainer, &MapContainer::mapReady, this, [this]() {
        QTimer::singleShot(2000, this, [this]() {
            qDebug() << "[AUTO-ANN] Starting auto annotation test...";

            QVector<MapAnnotation> anns;
            MapAnnotation a;
            a.id = "auto-test-fr";
            a.latitude = 36.7538;
            a.longitude = 3.0588;
            a.title = QStringLiteral("Alger - Capitale");
            a.iconName = "red-marker";
            anns.append(a);

            qDebug() << "[AUTO-ANN] Calling setAnnotations...";
            m_mapContainer->setAnnotations(anns);

            qDebug() << "[AUTO-ANN] Calling registerAnnotationIcons...";
            QMap<QString, QImage> icons;
            QImage marker(32, 32, QImage::Format_ARGB32);
            marker.fill(Qt::red);
            icons["red-marker"] = marker;
            m_mapContainer->registerAnnotationIcons(icons);

            qDebug() << "[AUTO-ANN] Calling rebuildSource by re-setting annotations...";
            m_mapContainer->setAnnotations(anns);

            qDebug() << "[AUTO-ANN] Setting center and zoom...";
            m_mapContainer->setCenter(36.7538, 3.0588);
            m_mapContainer->setZoom(14.0);

            qDebug() << "[AUTO-ANN] Done! Annotation should be visible now.";
        });
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

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_mapContainer && event->type() == QEvent::Resize) {
        repositionScaleBar();
    }
    return QMainWindow::eventFilter(watched, event);
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
