#include "controlpanelwidget.h"

#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>

/**
 * @brief 控制面板构造函数
 *
 * 初始化三个垂直滑块（缩放、旋转、倾斜）及其对应的标签。
 * 设置滑块范围、UI样式和信号连接，建立用户交互与地图状态的双向绑定。
 */
ControlPanelWidget::ControlPanelWidget(QWidget *parent)
    : QWidget(parent)
    , m_zoomSlider(new QSlider(Qt::Vertical))
    , m_bearingSlider(new QSlider(Qt::Vertical))
    , m_tiltSlider(new QSlider(Qt::Vertical))
    , m_zoomLabel(new QLabel(QStringLiteral("缩放 Z0")))
    , m_bearingLabel(new QLabel(QStringLiteral("旋转 0°")))
    , m_tiltLabel(new QLabel(QStringLiteral("倾斜 0°")))
{
    // 固定宽度 70px，确保面板紧凑不占用过多地图显示区域
    setFixedWidth(70);
    setAttribute(Qt::WA_StyledBackground);

    // UI 样式设计：
    // - 半透明黑色背景 (rgba(0,0,0,128))：在地图上方保持可见性，同时不遮挡地图内容
    // - 圆角边框 (border-radius: 8px)：视觉更柔和，与现代UI风格保持一致
    setStyleSheet(QStringLiteral("background-color: rgba(0,0,0,128); border-radius: 8px;"));

    // ===== 滑块范围初始化 =====
    // 缩放滑块：范围 0-200，映射到 0.0-20.0（除以 10.0）
    // 使用整数滑块存储小数精度，步进值为 0.1
    m_zoomSlider->setRange(0, 200);
    m_zoomSlider->setValue(0);

    // 旋转滑块：范围 0-360，直接映射到 0°-360°
    m_bearingSlider->setRange(0, 360);
    m_bearingSlider->setValue(0);

    // 倾斜滑块：范围 0-60，直接映射到 0°-60°
    m_tiltSlider->setRange(0, 60);
    m_tiltSlider->setValue(0);

    // 标签样式：白色文字、12px 字号，在深色背景上保持清晰可读
    const QString labelStyle = QStringLiteral("color: white; font-size: 12px;");
    m_zoomLabel->setStyleSheet(labelStyle);
    m_bearingLabel->setStyleSheet(labelStyle);
    m_tiltLabel->setStyleSheet(labelStyle);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_bearingLabel->setAlignment(Qt::AlignCenter);
    m_tiltLabel->setAlignment(Qt::AlignCenter);

    // 垂直布局：标签在上，滑块在下，紧凑排列
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(5, 5, 5, 5);

    layout->addWidget(m_zoomLabel);
    layout->addWidget(m_zoomSlider, 1);
    layout->addWidget(m_bearingLabel);
    layout->addWidget(m_bearingSlider, 1);
    layout->addWidget(m_tiltLabel);
    layout->addWidget(m_tiltSlider, 1);

    // ===== 信号连接：滑块值变化 → 发出信号通知地图更新 =====
    // 值映射逻辑：
    // - Zoom: slider value / 10.0 = zoom level（整数滑块存储小数）
    // - Bearing: slider value = bearing degrees（直接映射）
    // - Tilt: slider value = tilt degrees（直接映射）

    connect(m_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        double zoom = value / 10.0;  // 整数映射为小数：0-200 → 0.0-20.0
        m_zoomLabel->setText(QStringLiteral("缩放 Z%1").arg(static_cast<int>(zoom)));
        emit zoomChanged(zoom);
    });

    connect(m_bearingSlider, &QSlider::valueChanged, this, [this](int value) {
        m_bearingLabel->setText(QStringLiteral("旋转 %1°").arg(value));
        emit bearingChanged(static_cast<double>(value));
    });

    connect(m_tiltSlider, &QSlider::valueChanged, this, [this](int value) {
        m_tiltLabel->setText(QStringLiteral("倾斜 %1°").arg(value));
        emit tiltChanged(static_cast<double>(value));
    });
}

/**
 * @brief 设置缩放值（外部调用，如地图状态更新）
 *
 * 双向绑定机制说明：
 * 1. 用户拖动滑块 → 触发 valueChanged → 发出 zoomChanged 信号 → 地图更新
 * 2. 地图更新后 → 调用 setZoomValue → 更新滑块位置 → 保持UI同步
 *
 * blockSignals(true/false) 的作用：
 * - 在 setValue() 之前 blockSignals(true)，防止 setValue 触发 valueChanged
 * - 否则会造成循环：setValue → valueChanged → zoomChanged → setZoomValue → ...
 * - setValue 完成后恢复 blockSignals(false)，确保用户后续操作正常触发信号
 */
void ControlPanelWidget::setZoomValue(double zoom)
{
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(static_cast<int>(zoom * 10));  // 小数映射为整数：zoom * 10
    m_zoomSlider->blockSignals(false);
    m_zoomLabel->setText(QStringLiteral("缩放 Z%1").arg(static_cast<int>(zoom)));
}

/**
 * @brief 设置旋转值（外部调用，如地图状态更新）
 *
 * 双向绑定机制同 setZoomValue：
 * 使用 blockSignals 防止 setValue 触发 valueChanged 造成信号循环。
 */
void ControlPanelWidget::setBearingValue(double bearing)
{
    m_bearingSlider->blockSignals(true);
    m_bearingSlider->setValue(static_cast<int>(bearing));  // 直接映射：double → int
    m_bearingSlider->blockSignals(false);
    m_bearingLabel->setText(QStringLiteral("旋转 %1°").arg(static_cast<int>(bearing)));
}

/**
 * @brief 设置倾斜值（外部调用，如地图状态更新）
 *
 * 双向绑定机制同 setZoomValue：
 * 使用 blockSignals 防止 setValue 触发 valueChanged 造成信号循环。
 */
void ControlPanelWidget::setTiltValue(double tilt)
{
    m_tiltSlider->blockSignals(true);
    m_tiltSlider->setValue(static_cast<int>(tilt));  // 直接映射：double → int
    m_tiltSlider->blockSignals(false);
    m_tiltLabel->setText(QStringLiteral("倾斜 %1°").arg(static_cast<int>(tilt)));
}
