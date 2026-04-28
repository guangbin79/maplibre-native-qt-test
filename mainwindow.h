#pragma once
#include <QMainWindow>
#include <QCheckBox>

class MapContainer;
class ScaleBarWidget;
class ControlPanelWidget;

/**
 * @brief 应用程序主窗口 - 架构概述
 *
 * MainWindow 是整个地图查看器应用程序的主窗口，负责协调三个核心组件：
 *
 * 组件架构：
 * ┌─────────────────────────────────────────────┐
 * │  MainWindow                                  │
 * │  ┌──────────────────────┬─────────────────┐ │
 * │  │ MapContainer         │ ControlPanel    │ │
 * │  │ (地图显示区域)        │ (控制面板)       │ │
 * │  │                      │                 │ │
 * │  │  ┌──────────────┐    │                 │ │
 * │  │  │ ScaleBar     │    │                 │ │
 * │  │  │ (比例尺)      │    │                 │ │
 * │  │  └──────────────┘    │                 │ │
 * │  └──────────────────────┴─────────────────┘ │
 * └─────────────────────────────────────────────┘
 *
 * 组件职责：
 * - MapContainer: 地图渲染容器，基于 QMapLibre 显示地图瓦片
 * - ControlPanelWidget: 控制面板，提供缩放、旋转、倾斜的滑块控制
 * - ScaleBarWidget: 比例尺组件，显示当前地图比例
 *
 * Signal-Slot 数据流：
 * 1. ControlPanel → Map: 用户通过滑块调整地图状态
 *    (zoomChanged, bearingChanged, tiltChanged)
 * 2. Map → ControlPanel: 地图状态变化反馈到控制面板
 *    (zoomChanged, bearingChanged, tiltChanged)
 * 3. Map → ScaleBar: 地图缩放/中心变化时更新比例尺
 *    (zoomChanged, centerChanged)
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 设置手势状态，控制非地图控件的更新
     * 在触摸手势期间禁用 ControlPanel 和 ScaleBar 的更新，
     * 避免 mapChanged 信号触发大量 QWidget 重绘，减少 CPU 竞争。
     */
    void setGestureActive(bool active);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    /**
     * @brief 重新定位比例尺组件
     * 在 MapContainer 尺寸变化时，将 ScaleBar 定位到左下角
     */
    void repositionScaleBar();

    /// 地图容器 - 负责地图渲染和交互
    MapContainer *m_mapContainer;
    /// 比例尺组件 - 显示当前地图比例
    ScaleBarWidget *m_scaleBar;
    /// 控制面板 - 提供地图参数调节滑块
    ControlPanelWidget *m_controlPanel;
    QCheckBox *m_annotationLayerToggle;
    QCheckBox *m_routeLayerToggle;
};
