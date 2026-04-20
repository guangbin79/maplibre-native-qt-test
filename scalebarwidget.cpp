#include "scalebarwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <cmath>

ScaleBarWidget::ScaleBarWidget(QWidget *parent)
    : QWidget(parent)
    , m_niceDistances({1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000,
                       10000, 20000, 50000, 100000, 200000, 500000, 1000000})
{
    setFixedSize(150, 40);
}

void ScaleBarWidget::updateScale(double latitude, double zoomLevel)
{
    m_latitude = latitude;
    m_zoomLevel = zoomLevel;
    update();
}

void ScaleBarWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Web Mercator: meters per pixel
    const double metersPerPixel = 156543.03392 *
        std::cos(m_latitude * M_PI / 180.0) / std::pow(2.0, m_zoomLevel);

    // Select nice distance
    const double targetWidth = 100.0;
    const double targetMeters = metersPerPixel * targetWidth;
    double niceDistance = m_niceDistances.back();
    for (int d : m_niceDistances) {
        if (d >= targetMeters * 0.4) {
            niceDistance = d;
            break;
        }
    }

    const double barWidth = niceDistance / metersPerPixel;

    // Format distance text
    QString distanceText = niceDistance >= 1000
        ? QString("%1 km").arg(static_cast<int>(niceDistance / 1000))
        : QString("%1 m").arg(static_cast<int>(niceDistance));

    // Font setup
    QFont font = painter.font();
    font.setPixelSize(11);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::black);

    // Distance label at top-left
    const int textY = 8;
    painter.drawText(0, textY, distanceText);

    // Bar position
    const int barY = textY + 4;
    const int barH = 4;
    const int capExtra = 4;

    // Horizontal bar (rounded)
    painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, barY, static_cast<int>(barWidth), barH, 2, 2);

    // Left end cap
    painter.drawRect(0, barY - capExtra / 2, 2, barH + capExtra);

    // Right end cap
    painter.drawRect(static_cast<int>(barWidth) - 2, barY - capExtra / 2, 2, barH + capExtra);

    // Zoom indicator to the right of the bar
    painter.setPen(Qt::black);
    QString zoomText = QString("Z%1").arg(static_cast<int>(m_zoomLevel));
    painter.drawText(static_cast<int>(barWidth) + 6, textY, zoomText);
}
