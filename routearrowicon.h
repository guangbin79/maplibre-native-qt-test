#pragma once

#include <QColor>
#include <QImage>
#include <QString>

class RouteArrowIcon
{
public:
    static QImage generateArrowIcon(const QColor& color, int baseSize = 24);
    static QString iconKeyForColor(const QColor& color);
};
