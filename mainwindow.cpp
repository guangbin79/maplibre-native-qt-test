#include "mainwindow.h"
#include "mapcontainer.h"
#include "scalebarwidget.h"
#include "controlpanelwidget.h"
#include <QMapLibre/Map>
#include <QHBoxLayout>
#include <QResizeEvent>

/**
 * @brief 主窗口构造函数 - UI布局与信号连接初始化
 *
 * 初始化流程：
 * 1. 创建中央部件和水平布局
 * 2. 创建三个核心组件并添加到布局
 * 3. 建立组件间的信号-槽连接
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mapContainer(nullptr)
    , m_scaleBar(nullptr)
    , m_controlPanel(nullptr)
    , m_annotationLayerToggle(nullptr)
    , m_routeLayerToggle(nullptr)
{
    setWindowTitle(QStringLiteral("Map Viewer"));
#ifndef IS_ANDROID
    resize(1200, 800);
#endif

    // ── 中央部件设置 ────────────────────────────────────────
    auto *central = new QWidget(this);
    setCentralWidget(central);

    // ── UI 布局结构 ─────────────────────────────────────────
    // 使用 QHBoxLayout 实现左右分栏：
    // ┌──────────────────────────────┬──────────────┐
    // │ MapContainer (stretch=1)     │ ControlPanel │
    // │ 自适应填充剩余空间            │ fixed 70px   │
    // └──────────────────────────────┴──────────────┘
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 地图容器 - 占据左侧所有可用空间 (stretch=1)
    MapContainer::MapConfig config;
    config.styleUrl = QStringLiteral("http://127.0.0.1:4943/styles/day/style.json?schema=hxmap");
    m_mapContainer = new MapContainer(config, central);
    layout->addWidget(m_mapContainer, 1);

    // 控制面板 - 固定宽度 70px (stretch=0)
    m_controlPanel = new ControlPanelWidget(central);
    m_controlPanel->setFixedWidth(70);
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
