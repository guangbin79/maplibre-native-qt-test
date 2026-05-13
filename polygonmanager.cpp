#include "polygonmanager.h"
#include "polygongeojsonbuilder.h"
#include <QMapLibre/Map>

PolygonManager::PolygonManager(QMapLibre::Map* map, QObject* parent)
    : QObject(parent), m_map(map) {}

void PolygonManager::setMapReady(bool ready) { m_ready = ready; }

QStringList PolygonManager::allPolygonIds() const {
    QStringList ids;
    for (const auto& poly : m_polygons) {
        if (!ids.contains(poly.polygonId))
            ids.append(poly.polygonId);
    }
    return ids;
}

QStringList PolygonManager::visiblePolygonIds() const {
    return m_visiblePolygonIds;
}

bool PolygonManager::boundingBoxForPolygon(const QString& polygonId,
                                            QMapLibre::Coordinate& sw,
                                            QMapLibre::Coordinate& ne) const {
    double minLat = 90.0, maxLat = -90.0;
    double minLon = 180.0, maxLon = -180.0;
    bool found = false;

    for (const auto& poly : m_polygons) {
        if (poly.polygonId != polygonId) continue;
        found = true;
        for (const auto& coord : poly.coordinates) {
            minLat = qMin(minLat, coord.first);
            maxLat = qMax(maxLat, coord.first);
            minLon = qMin(minLon, coord.second);
            maxLon = qMax(maxLon, coord.second);
        }
    }

    if (!found) return false;

    double latSpan = qMax(maxLat - minLat, 0.001);
    double lonSpan = qMax(maxLon - minLon, 0.001);

    double latPad = qMax(latSpan * 0.1, 0.005);
    double lonPad = qMax(lonSpan * 0.1, 0.005);

    sw = QMapLibre::Coordinate(qMax(-90.0, minLat - latPad),
                                qMax(-180.0, minLon - lonPad));
    ne = QMapLibre::Coordinate(qMin(90.0, maxLat + latPad),
                                qMin(180.0, maxLon + lonPad));
    return true;
}

void PolygonManager::setPolygons(const QVector<MapPolygon>& polygons) {
    if (!m_ready) return;
    clearPolygons();
    m_polygons = polygons;
    m_visiblePolygonIds = allPolygonIds();
    ensureLayerSetup();
    rebuildSource();
}

void PolygonManager::addPolygon(const MapPolygon& polygon) {
    if (!m_ready) return;
    m_polygons.append(polygon);
    if (!m_visiblePolygonIds.contains(polygon.polygonId))
        m_visiblePolygonIds.append(polygon.polygonId);
    ensureLayerSetup();
    rebuildSource();
}

void PolygonManager::addPolygons(const QVector<MapPolygon>& polygons) {
    if (!m_ready) return;
    for (const auto& poly : polygons) {
        m_polygons.append(poly);
        if (!m_visiblePolygonIds.contains(poly.polygonId))
            m_visiblePolygonIds.append(poly.polygonId);
    }
    ensureLayerSetup();
    rebuildSource();
}

void PolygonManager::removePolygon(const QString& id) {
    if (!m_ready) return;
    auto it = std::find_if(m_polygons.begin(), m_polygons.end(),
        [&id](const MapPolygon& p) { return p.id == id; });
    if (it == m_polygons.end()) return;
    QString polygonId = it->polygonId;
    m_polygons.erase(it);
    bool hasPolygon = std::any_of(m_polygons.begin(), m_polygons.end(),
        [&polygonId](const MapPolygon& p) { return p.polygonId == polygonId; });
    if (!hasPolygon)
        m_visiblePolygonIds.removeAll(polygonId);
    rebuildSource();
    updateFilter();
}

void PolygonManager::removePolygons(const QStringList& ids) {
    if (!m_ready) return;
    for (const QString& id : ids) {
        auto it = std::find_if(m_polygons.begin(), m_polygons.end(),
            [&id](const MapPolygon& p) { return p.id == id; });
        if (it == m_polygons.end()) continue;
        QString polygonId = it->polygonId;
        m_polygons.erase(it);
        bool hasPolygon = std::any_of(m_polygons.begin(), m_polygons.end(),
            [&polygonId](const MapPolygon& p) { return p.polygonId == polygonId; });
        if (!hasPolygon)
            m_visiblePolygonIds.removeAll(polygonId);
    }
    rebuildSource();
    updateFilter();
}

void PolygonManager::clearPolygons() {
    if (m_layerSetup && m_map) {
        m_map->removeLayer("polygons-labels");
        m_map->removeLayer("polygons-stroke-dashed");
        m_map->removeLayer("polygons-stroke-solid");
        m_map->removeLayer("polygons-fill");
        m_map->removeSource("polygons");
        m_layerSetup = false;
    }
    m_polygons.clear();
    m_visiblePolygonIds.clear();
}

void PolygonManager::ensureLayerSetup() {
    if (m_layerSetup) return;
    if (!m_ready || !m_map) return;

    QByteArray geojson = PolygonGeoJsonBuilder::buildFeatureCollection(m_polygons);

    m_map->addSource("polygons", QVariantMap{{"type", "geojson"}, {"data", geojson}});

    // Fill layer
    m_map->addLayer("polygons-fill", QVariantMap{{"type", "fill"}, {"source", "polygons"}});
    m_map->setPaintProperty("polygons-fill", "fill-color", QVariantList{"get", "fillColor"});
    m_map->setPaintProperty("polygons-fill", "fill-opacity", QVariantList{"get", "fillOpacity"});
    m_map->setLayoutProperty("polygons-fill", "filter",
        QVariantList{"==", QVariantList{"get", "geometryType"}, "fill"});

    // Solid stroke layer
    m_map->addLayer("polygons-stroke-solid", QVariantMap{{"type", "line"}, {"source", "polygons"}});
    m_map->setLayoutProperty("polygons-stroke-solid", "line-cap", "round");
    m_map->setLayoutProperty("polygons-stroke-solid", "line-join", "round");
    m_map->setPaintProperty("polygons-stroke-solid", "line-color", QVariantList{"get", "strokeColor"});
    m_map->setPaintProperty("polygons-stroke-solid", "line-width", QVariantList{"get", "strokeWidth"});
    m_map->setLayoutProperty("polygons-stroke-solid", "filter",
        QVariantList{"all", QVariantList{"==", QVariantList{"get", "geometryType"}, "outline"},
                     QVariantList{"==", QVariantList{"get", "strokeType"}, "solid"}});

    // Dashed stroke layer
    m_map->addLayer("polygons-stroke-dashed", QVariantMap{{"type", "line"}, {"source", "polygons"}});
    m_map->setLayoutProperty("polygons-stroke-dashed", "line-cap", "round");
    m_map->setLayoutProperty("polygons-stroke-dashed", "line-join", "round");
    m_map->setPaintProperty("polygons-stroke-dashed", "line-color", QVariantList{"get", "strokeColor"});
    m_map->setPaintProperty("polygons-stroke-dashed", "line-width", QVariantList{"get", "strokeWidth"});
    m_map->setPaintProperty("polygons-stroke-dashed", "line-dasharray", QVariantList{1, 2});
    m_map->setLayoutProperty("polygons-stroke-dashed", "filter",
        QVariantList{"all", QVariantList{"==", QVariantList{"get", "geometryType"}, "outline"},
                     QVariantList{"==", QVariantList{"get", "strokeType"}, "dashed"}});

    // Label layer
    m_map->addLayer("polygons-labels", QVariantMap{{"type", "symbol"}, {"source", "polygons"}});
    m_map->setLayoutProperty("polygons-labels", "symbol-placement", "point");
    m_map->setLayoutProperty("polygons-labels", "text-field", "{title}");
    m_map->setLayoutProperty("polygons-labels", "text-size", 12);
    m_map->setLayoutProperty("polygons-labels", "text-anchor", "center");
    m_map->setLayoutProperty("polygons-labels", "filter",
        QVariantList{"==", QVariantList{"get", "geometryType"}, "label"});

    m_layerSetup = true;
}

void PolygonManager::rebuildSource() {
    if (!m_layerSetup || !m_ready || !m_map) return;
    QByteArray geojson = PolygonGeoJsonBuilder::buildFeatureCollection(m_polygons);
    m_map->updateSource("polygons", QVariantMap{{"data", geojson}});
}

void PolygonManager::updateFilter() {
    if (!m_layerSetup || !m_ready || !m_map) return;

    QVariantList fillTypeFilter = {"==", QVariantList{"get", "geometryType"}, "fill"};
    QVariantList solidStrokeFilter = {"all", QVariantList{"==", QVariantList{"get", "geometryType"}, "outline"},
                                             QVariantList{"==", QVariantList{"get", "strokeType"}, "solid"}};
    QVariantList dashedStrokeFilter = {"all", QVariantList{"==", QVariantList{"get", "geometryType"}, "outline"},
                                              QVariantList{"==", QVariantList{"get", "strokeType"}, "dashed"}};
    QVariantList labelTypeFilter = {"==", QVariantList{"get", "geometryType"}, "label"};

    if (m_visiblePolygonIds.isEmpty()) {
        QVariantList hideFilter = {"==", "1", "0"};
        m_map->setLayoutProperty("polygons-fill", "filter", hideFilter);
        m_map->setLayoutProperty("polygons-stroke-solid", "filter", hideFilter);
        m_map->setLayoutProperty("polygons-stroke-dashed", "filter", hideFilter);
        m_map->setLayoutProperty("polygons-labels", "filter", hideFilter);
    } else if (m_visiblePolygonIds.size() == allPolygonIds().size()) {
        m_map->setLayoutProperty("polygons-fill", "filter", fillTypeFilter);
        m_map->setLayoutProperty("polygons-stroke-solid", "filter", solidStrokeFilter);
        m_map->setLayoutProperty("polygons-stroke-dashed", "filter", dashedStrokeFilter);
        m_map->setLayoutProperty("polygons-labels", "filter", labelTypeFilter);
    } else {
        QVariantList visibleIds;
        for (const auto& id : m_visiblePolygonIds) visibleIds << id;
        QVariantList polygonIdFilter = {"in", QVariantList{"get", "polygonId"}, QVariantList{"literal", visibleIds}};

        m_map->setLayoutProperty("polygons-fill", "filter",
            QVariantList{"all", fillTypeFilter, polygonIdFilter});
        m_map->setLayoutProperty("polygons-stroke-solid", "filter",
            QVariantList{"all", solidStrokeFilter, polygonIdFilter});
        m_map->setLayoutProperty("polygons-stroke-dashed", "filter",
            QVariantList{"all", dashedStrokeFilter, polygonIdFilter});
        m_map->setLayoutProperty("polygons-labels", "filter",
            QVariantList{"all", labelTypeFilter, polygonIdFilter});
    }
}

void PolygonManager::setVisiblePolygonIds(const QStringList& polygonIds) {
    m_visiblePolygonIds = polygonIds;
    updateFilter();
}

void PolygonManager::showAllPolygons() {
    m_visiblePolygonIds = allPolygonIds();
    updateFilter();
}

void PolygonManager::hideAllPolygons() {
    m_visiblePolygonIds.clear();
    updateFilter();
}
