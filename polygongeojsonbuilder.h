#ifndef POLYGONGEOJSONBUILDER_H
#define POLYGONGEOJSONBUILDER_H

#include <QByteArray>
#include <QVector>
#include "mappolygon.h"

class PolygonGeoJsonBuilder {
public:
    static QByteArray buildFeatureCollection(
        const QVector<MapPolygon>& polygons);
};

#endif // POLYGONGEOJSONBUILDER_H
