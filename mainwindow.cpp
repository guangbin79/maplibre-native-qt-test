#include "mainwindow.h"
#include "mapcontainer.h"
#include "scalebarwidget.h"
#include "controlpanelwidget.h"
#include <QMapLibre/Map>
#include <QHBoxLayout>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mapContainer(nullptr)
    , m_scaleBar(nullptr)
    , m_controlPanel(nullptr)
{
    setWindowTitle(QStringLiteral("Map Viewer"));
    resize(1200, 800);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_mapContainer = new MapContainer(central);
    layout->addWidget(m_mapContainer, 1);

    m_controlPanel = new ControlPanelWidget(central);
    m_controlPanel->setFixedWidth(70);
    layout->addWidget(m_controlPanel, 0);

    m_scaleBar = new ScaleBarWidget(m_mapContainer);
    repositionScaleBar();

    // ── Signal-slot wiring ──────────────────────────────────

    // ControlPanel → Map (user adjusts sliders)
    connect(m_controlPanel, &ControlPanelWidget::zoomChanged,
            this, [this](double zoom) { m_mapContainer->map()->setZoom(zoom); });
    connect(m_controlPanel, &ControlPanelWidget::bearingChanged,
            this, [this](double bearing) { m_mapContainer->map()->setBearing(bearing); });
    connect(m_controlPanel, &ControlPanelWidget::tiltChanged,
            this, [this](double tilt) { m_mapContainer->map()->setPitch(tilt); });

    // Map → ControlPanel (map state changes from gestures/mouse)
    connect(m_mapContainer, &MapContainer::zoomChanged,
            m_controlPanel, &ControlPanelWidget::setZoomValue);
    connect(m_mapContainer, &MapContainer::bearingChanged,
            m_controlPanel, &ControlPanelWidget::setBearingValue);
    connect(m_mapContainer, &MapContainer::tiltChanged,
            m_controlPanel, &ControlPanelWidget::setTiltValue);

    // Map → ScaleBar (update scale on zoom/center change)
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    repositionScaleBar();
}

void MainWindow::repositionScaleBar()
{
    m_scaleBar->move(16, m_mapContainer->height() - m_scaleBar->height() - 24);
}
