#pragma once
#include <QWidget>

class QSlider;
class QLabel;

/**
 * @brief 控制面板控件 - 地图视角控制器
 *
 * 本控件提供三个垂直滑块，用于实时控制地图的缩放级别、旋转角度和倾斜角度。
 * 采用双向绑定设计：用户操作滑块会触发信号通知地图更新，地图状态变化也会
 * 反向更新滑块位置，保持UI与地图状态同步。
 *
 * 三个控制维度：
 * - 缩放 (Zoom): 滑块范围 0-200，映射到 0.0-20.0，步进 0.1
 * - 旋转 (Bearing): 滑块范围 0-360，映射到 0°-360°
 * - 倾斜 (Tilt): 滑块范围 0-60，映射到 0°-60°
 */
class ControlPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ControlPanelWidget(QWidget *parent = nullptr);

signals:
    /**
     * @brief 缩放级别改变时发出
     * @param zoom 缩放级别，范围 0.0 - 20.0
     * @note 当用户拖动缩放滑块时触发，或在 setZoomValue() 被调用后触发
     */
    void zoomChanged(double zoom);

    /**
     * @brief 旋转角度改变时发出
     * @param bearing 旋转角度（度），范围 0.0 - 360.0
     * @note 当用户拖动旋转滑块时触发，或在 setBearingValue() 被调用后触发
     */
    void bearingChanged(double bearing);

    /**
     * @brief 倾斜角度改变时发出
     * @param tilt 倾斜角度（度），范围 0.0 - 60.0
     * @note 当用户拖动倾斜滑块时触发，或在 setTiltValue() 被调用后触发
     */
    void tiltChanged(double tilt);

public slots:
    /**
     * @brief 设置缩放滑块值
     * @param zoom 缩放级别，范围 0.0 - 20.0
     * @note 用于接收地图状态更新，内部使用 blockSignals 防止循环触发
     */
    void setZoomValue(double zoom);

    /**
     * @brief 设置旋转滑块值
     * @param bearing 旋转角度（度），范围 0.0 - 360.0
     * @note 用于接收地图状态更新，内部使用 blockSignals 防止循环触发
     */
    void setBearingValue(double bearing);

    /**
     * @brief 设置倾斜滑块值
     * @param tilt 倾斜角度（度），范围 0.0 - 60.0
     * @note 用于接收地图状态更新，内部使用 blockSignals 防止循环触发
     */
    void setTiltValue(double tilt);

private:
    QSlider *m_zoomSlider;      ///< 缩放滑块，范围 0-200
    QSlider *m_bearingSlider;   ///< 旋转滑块，范围 0-360
    QSlider *m_tiltSlider;      ///< 倾斜滑块，范围 0-60
    QLabel *m_zoomLabel;        ///< 缩放值显示标签
    QLabel *m_bearingLabel;     ///< 旋转值显示标签
    QLabel *m_tiltLabel;        ///< 倾斜值显示标签
};
