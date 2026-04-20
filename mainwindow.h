#pragma once
#include <QMainWindow>

class MapContainer;
class ScaleBarWidget;
class ControlPanelWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void repositionScaleBar();

    MapContainer *m_mapContainer;
    ScaleBarWidget *m_scaleBar;
    ControlPanelWidget *m_controlPanel;
};
