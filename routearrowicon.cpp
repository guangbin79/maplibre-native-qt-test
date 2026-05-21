#include "routearrowicon.h"

#include <QPainter>
#include <QPainterPath>

QImage RouteArrowIcon::generateArrowIcon(const QColor& color, int baseSize)
{
    if (baseSize <= 0)
        baseSize = 24;

    QImage image(baseSize, baseSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal margin = baseSize * 0.1;
    const qreal cx = baseSize / 2.0;

    QPainterPath path;
    path.moveTo(cx, margin);                              // top vertex
    path.lineTo(margin, baseSize - margin);               // bottom-left
    path.lineTo(baseSize - margin, baseSize - margin);    // bottom-right
    path.closeSubpath();

    painter.setBrush(color.isValid() ? color : QColor(Qt::black));
    painter.setPen(Qt::NoPen);
    painter.drawPath(path);

    return image;
}

QString RouteArrowIcon::iconKeyForColor(const QColor& color)
{
    return QStringLiteral("route-arrow-") + color.name();
}
