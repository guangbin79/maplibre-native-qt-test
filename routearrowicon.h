#pragma once

#include <QColor>
#include <QImage>
#include <QString>

class RouteArrowIcon
{
public:
    static QImage generateArrowIcon(const QColor& color, int baseSize = 8);
    static QString iconKeyForColor(const QColor& color);
};
