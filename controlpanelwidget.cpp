#include "controlpanelwidget.h"

#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>

ControlPanelWidget::ControlPanelWidget(QWidget *parent)
    : QWidget(parent)
    , m_zoomSlider(new QSlider(Qt::Vertical))
    , m_bearingSlider(new QSlider(Qt::Vertical))
    , m_tiltSlider(new QSlider(Qt::Vertical))
    , m_zoomLabel(new QLabel(QStringLiteral("缩放 Z0")))
    , m_bearingLabel(new QLabel(QStringLiteral("旋转 0°")))
    , m_tiltLabel(new QLabel(QStringLiteral("倾斜 0°")))
{
    setFixedWidth(70);
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet(QStringLiteral("background-color: rgba(0,0,0,128); border-radius: 8px;"));

    m_zoomSlider->setRange(0, 200);
    m_zoomSlider->setValue(0);

    m_bearingSlider->setRange(0, 360);
    m_bearingSlider->setValue(0);

    m_tiltSlider->setRange(0, 60);
    m_tiltSlider->setValue(0);


    const QString labelStyle = QStringLiteral("color: white; font-size: 12px;");
    m_zoomLabel->setStyleSheet(labelStyle);
    m_bearingLabel->setStyleSheet(labelStyle);
    m_tiltLabel->setStyleSheet(labelStyle);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_bearingLabel->setAlignment(Qt::AlignCenter);
    m_tiltLabel->setAlignment(Qt::AlignCenter);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(5, 5, 5, 5);

    layout->addWidget(m_zoomLabel);
    layout->addWidget(m_zoomSlider, 1);
    layout->addWidget(m_bearingLabel);
    layout->addWidget(m_bearingSlider, 1);
    layout->addWidget(m_tiltLabel);
    layout->addWidget(m_tiltSlider, 1);

    connect(m_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        double zoom = value / 10.0;
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

void ControlPanelWidget::setZoomValue(double zoom)
{
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(static_cast<int>(zoom * 10));
    m_zoomSlider->blockSignals(false);
    m_zoomLabel->setText(QStringLiteral("缩放 Z%1").arg(static_cast<int>(zoom)));
}

void ControlPanelWidget::setBearingValue(double bearing)
{
    m_bearingSlider->blockSignals(true);
    m_bearingSlider->setValue(static_cast<int>(bearing));
    m_bearingSlider->blockSignals(false);
    m_bearingLabel->setText(QStringLiteral("旋转 %1°").arg(static_cast<int>(bearing)));
}

void ControlPanelWidget::setTiltValue(double tilt)
{
    m_tiltSlider->blockSignals(true);
    m_tiltSlider->setValue(static_cast<int>(tilt));
    m_tiltSlider->blockSignals(false);
    m_tiltLabel->setText(QStringLiteral("倾斜 %1°").arg(static_cast<int>(tilt)));
}
