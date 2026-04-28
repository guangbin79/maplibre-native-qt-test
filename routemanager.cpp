#include "routemanager.h"
#include "routegeojsonbuilder.h"
#include <QMapLibre/Map>

RouteManager::RouteManager(QMapLibre::Map* map, QObject* parent)
    : QObject(parent), m_map(map) {}

void RouteManager::setMapReady(bool ready) { m_ready = ready; }

QStringList RouteManager::allRouteIds() const {
    QStringList ids;
    for (const auto& seg : m_segments) {
        if (!ids.contains(seg.routeId))
            ids.append(seg.routeId);
    }
    return ids;
}

QStringList RouteManager::visibleRouteIds() const {
    return m_visibleRouteIds;
}

void RouteManager::setSegments(const QVector<MapRouteSegment>& segments) {
    if (!m_ready) return;
    clearSegments();
    m_segments = segments;
    m_visibleRouteIds = allRouteIds();
    ensureLayerSetup();
    rebuildSource();
}

void RouteManager::addRouteSegment(const MapRouteSegment& segment) {
    if (!m_ready) return;
    m_segments.append(segment);
    if (!m_visibleRouteIds.contains(segment.routeId))
        m_visibleRouteIds.append(segment.routeId);
    ensureLayerSetup();
    rebuildSource();
}

void RouteManager::addRouteSegments(const QVector<MapRouteSegment>& segments) {
    if (!m_ready) return;
    for (const auto& seg : segments) {
        m_segments.append(seg);
        if (!m_visibleRouteIds.contains(seg.routeId))
            m_visibleRouteIds.append(seg.routeId);
    }
    ensureLayerSetup();
    rebuildSource();
}

void RouteManager::removeRouteSegment(const QString& id) {
    if (!m_ready) return;
    auto it = std::find_if(m_segments.begin(), m_segments.end(),
        [&id](const MapRouteSegment& s) { return s.id == id; });
    if (it == m_segments.end()) return;
    QString routeId = it->routeId;
    m_segments.erase(it);
    bool hasRoute = std::any_of(m_segments.begin(), m_segments.end(),
        [&routeId](const MapRouteSegment& s) { return s.routeId == routeId; });
    if (!hasRoute)
        m_visibleRouteIds.removeAll(routeId);
    rebuildSource();
    updateFilter();
}

void RouteManager::removeRouteSegments(const QStringList& ids) {
    if (!m_ready) return;
    for (const QString& id : ids) {
        auto it = std::find_if(m_segments.begin(), m_segments.end(),
            [&id](const MapRouteSegment& s) { return s.id == id; });
        if (it == m_segments.end()) continue;
        QString routeId = it->routeId;
        m_segments.erase(it);
        bool hasRoute = std::any_of(m_segments.begin(), m_segments.end(),
            [&routeId](const MapRouteSegment& s) { return s.routeId == routeId; });
        if (!hasRoute)
            m_visibleRouteIds.removeAll(routeId);
    }
    rebuildSource();
    updateFilter();
}

void RouteManager::clearSegments() {
    if (m_layerSetup && m_map) {
        m_map->removeLayer("routes-labels");
        m_map->removeLayer("routes-dashed");
        m_map->removeLayer("routes-solid");
        m_map->removeSource("routes");
        m_layerSetup = false;
    }
    m_segments.clear();
    m_visibleRouteIds.clear();
}

void RouteManager::ensureLayerSetup() {
    if (m_layerSetup) return;
    if (!m_ready || !m_map) return;

    QByteArray geojson = RouteGeoJsonBuilder::buildFeatureCollection(m_segments);

    m_map->addSource("routes", QVariantMap{{"type", "geojson"}, {"data", geojson}});

    m_map->addLayer("routes-solid", QVariantMap{{"type", "line"}, {"source", "routes"}});
    m_map->setLayoutProperty("routes-solid", "line-cap", "round");
    m_map->setLayoutProperty("routes-solid", "line-join", "round");
    m_map->setPaintProperty("routes-solid", "line-color", QVariantList{"get", "color"});
    m_map->setPaintProperty("routes-solid", "line-width", QVariantList{"get", "width"});
    m_map->setLayoutProperty("routes-solid", "filter",
        QVariantList{"==", QVariantList{"get", "lineType"}, "solid"});

    m_map->addLayer("routes-dashed", QVariantMap{{"type", "line"}, {"source", "routes"}});
    m_map->setLayoutProperty("routes-dashed", "line-cap", "round");
    m_map->setLayoutProperty("routes-dashed", "line-join", "round");
    m_map->setPaintProperty("routes-dashed", "line-color", QVariantList{"get", "color"});
    m_map->setPaintProperty("routes-dashed", "line-width", QVariantList{"get", "width"});
    m_map->setPaintProperty("routes-dashed", "line-dasharray", QVariantList{1, 2});
    m_map->setLayoutProperty("routes-dashed", "filter",
        QVariantList{"==", QVariantList{"get", "lineType"}, "dashed"});

    m_map->addLayer("routes-labels", QVariantMap{{"type", "symbol"}, {"source", "routes"}});
    m_map->setLayoutProperty("routes-labels", "symbol-placement", "line-center");
    m_map->setLayoutProperty("routes-labels", "text-field", "{title}");
    m_map->setLayoutProperty("routes-labels", "text-font", QStringList{"Roboto Regular"});
    m_map->setLayoutProperty("routes-labels", "text-size", 12);
    m_map->setLayoutProperty("routes-labels", "text-anchor", "center");
    m_map->setLayoutProperty("routes-labels", "filter",
        QVariantList{"!=", QVariantList{"get", "title"}, ""});

    m_layerSetup = true;
}

void RouteManager::rebuildSource() {
    if (!m_layerSetup || !m_ready || !m_map) return;
    QByteArray geojson = RouteGeoJsonBuilder::buildFeatureCollection(m_segments);
    m_map->updateSource("routes", QVariantMap{{"data", geojson}});
}

void RouteManager::updateFilter() {
    if (!m_layerSetup || !m_ready || !m_map) return;

    QVariantList solidTypeFilter = {"==", QVariantList{"get", "lineType"}, "solid"};
    QVariantList dashedTypeFilter = {"==", QVariantList{"get", "lineType"}, "dashed"};
    QVariantList labelTypeFilter = {"!=", QVariantList{"get", "title"}, ""};

    if (m_visibleRouteIds.isEmpty()) {
        QVariantList hideFilter = {"==", "1", "0"};
        m_map->setLayoutProperty("routes-solid", "filter", hideFilter);
        m_map->setLayoutProperty("routes-dashed", "filter", hideFilter);
        m_map->setLayoutProperty("routes-labels", "filter", hideFilter);
    } else if (m_visibleRouteIds.size() == allRouteIds().size()) {
        m_map->setLayoutProperty("routes-solid", "filter", solidTypeFilter);
        m_map->setLayoutProperty("routes-dashed", "filter", dashedTypeFilter);
        m_map->setLayoutProperty("routes-labels", "filter", labelTypeFilter);
    } else {
        QVariantList visibleIds;
        for (const auto& id : m_visibleRouteIds) visibleIds << id;
        QVariantList routeIdFilter = {"in", QVariantList{"get", "routeId"}, QVariantList{"literal", visibleIds}};

        m_map->setLayoutProperty("routes-solid", "filter",
            QVariantList{"all", solidTypeFilter, routeIdFilter});
        m_map->setLayoutProperty("routes-dashed", "filter",
            QVariantList{"all", dashedTypeFilter, routeIdFilter});
        m_map->setLayoutProperty("routes-labels", "filter",
            QVariantList{"all", labelTypeFilter, routeIdFilter});
    }
}

void RouteManager::setVisibleRouteIds(const QStringList& routeIds) {
    m_visibleRouteIds = routeIds;
    updateFilter();
}

void RouteManager::showAllRoutes() {
    m_visibleRouteIds = allRouteIds();
    updateFilter();
}

void RouteManager::hideAllRoutes() {
    m_visibleRouteIds.clear();
    updateFilter();
}
