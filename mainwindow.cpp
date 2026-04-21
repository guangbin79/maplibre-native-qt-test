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
{
    setWindowTitle(QStringLiteral("Map Viewer"));
    resize(1200, 800);

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
