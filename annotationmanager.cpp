#include "annotationmanager.h"
#include "geojsonbuilder.h"

#include <QMapLibre/Map>
#include <QGuiApplication>
#include <QScreen>
#include <QVariantList>
#include <QVariantMap>
#include <algorithm>

AnnotationManager::AnnotationManager(QMapLibre::Map* map, QObject* parent)
    : QObject(parent)
    , m_map(map)
{
}

void AnnotationManager::setMapReady(bool ready)
{
    m_ready = ready;
}

void AnnotationManager::setAnnotations(const QVector<MapAnnotation>& annotations,
                                         const QMap<QString, QImage>& icons)
{
    if (!m_ready)
        return;

    clearAnnotations();

    m_annotations = annotations;
    m_visibleIds = allIds();

    registerAllIcons(icons);

    ensureLayerSetup();

    rebuildSource();
}

void AnnotationManager::clearAnnotations()
{
    if (m_layerSetup && m_map) {
        m_map->removeLayer("annotations-layer");
        m_map->removeSource("annotations");
        m_layerSetup = false;
    }

    for (auto it = m_iconRefCount.begin(); it != m_iconRefCount.end(); ++it) {
        if (m_map)
            m_map->removeImage(it.key());
    }
    m_iconRefCount.clear();
    m_icons.clear();
    m_annotations.clear();
    m_visibleIds.clear();
}

void AnnotationManager::addAnnotation(const MapAnnotation& annotation, const QImage& icon)
{
    if (!m_ready)
        return;

    m_annotations.append(annotation);
    m_visibleIds.append(annotation.id);

    if (!icon.isNull()) {
        registerIcon(annotation.iconName, icon);
    } else if (!annotation.iconName.isEmpty()) {
        m_iconRefCount[annotation.iconName]++;
    }

    ensureLayerSetup();
    rebuildSource();
}

void AnnotationManager::addAnnotations(const QVector<MapAnnotation>& annotations,
                                         const QMap<QString, QImage>& icons)
{
    for (const auto& ann : annotations)
        addAnnotation(ann, icons.value(ann.iconName));
}

void AnnotationManager::removeAnnotation(const QString& id)
{
    auto it = std::find_if(m_annotations.begin(), m_annotations.end(),
        [&id](const MapAnnotation& ann) { return ann.id == id; });
    if (it == m_annotations.end())
        return;

    QString iconName = it->iconName;
    m_annotations.erase(it);
    m_visibleIds.removeAll(id);

    unregisterIcon(iconName);

    rebuildSource();
    updateFilter();
}

void AnnotationManager::removeAnnotations(const QStringList& ids)
{
    for (const auto& id : ids)
        removeAnnotation(id);
}

void AnnotationManager::setVisibleIds(const QStringList& ids)
{
    m_visibleIds = ids;
    updateFilter();
}

void AnnotationManager::showAllAnnotations()
{
    m_visibleIds = allIds();
    updateFilter();
}

void AnnotationManager::hideAllAnnotations()
{
    m_visibleIds.clear();
    updateFilter();
}

QStringList AnnotationManager::allIds() const
{
    QStringList ids;
    ids.reserve(m_annotations.size());
    for (const auto& ann : m_annotations)
        ids.append(ann.id);
    return ids;
}

QStringList AnnotationManager::visibleIds() const
{
    return m_visibleIds;
}

void AnnotationManager::ensureLayerSetup()
{
    if (m_layerSetup)
        return;
    if (!m_ready || !m_map)
        return;

    QByteArray geojson = GeoJsonBuilder::buildFeatureCollection(m_annotations);
    m_map->addSource("annotations", QVariantMap{
        {"type", "geojson"},
        {"data", geojson}
    });

    m_map->addLayer("annotations-layer", QVariantMap{
        {"id", "annotations-layer"},
        {"type", "symbol"},
        {"source", "annotations"}
    });

    m_map->setLayoutProperty("annotations-layer", "icon-image", "{icon}");
    m_map->setLayoutProperty("annotations-layer", "icon-size", 1.0);
    m_map->setLayoutProperty("annotations-layer", "icon-anchor", "bottom");
    m_map->setLayoutProperty("annotations-layer", "text-field", "{title}");
    m_map->setLayoutProperty("annotations-layer", "text-font", QStringList{"NotoSans-Regular"});
    m_map->setLayoutProperty("annotations-layer", "text-size", 12);
    m_map->setLayoutProperty("annotations-layer", "text-anchor", "top");
    m_map->setLayoutProperty("annotations-layer", "text-offset", QVariantList{0.0, 0.5});

    m_layerSetup = true;
}

void AnnotationManager::rebuildSource()
{
    if (!m_layerSetup || !m_ready || !m_map)
        return;

    QByteArray geojson = GeoJsonBuilder::buildFeatureCollection(m_annotations);
    m_map->updateSource("annotations", QVariantMap{{"data", geojson}});
}

void AnnotationManager::updateFilter()
{
    if (!m_layerSetup || !m_ready || !m_map)
        return;

    if (m_visibleIds.isEmpty()) {
        m_map->setLayoutProperty("annotations-layer", "filter",
            QVariantList{"==", "1", "0"});
    } else if (m_visibleIds.size() == static_cast<int>(m_annotations.size())) {
        m_map->setLayoutProperty("annotations-layer", "filter", QVariant());
    } else {
        QVariantList ids;
        for (const auto& id : m_visibleIds)
            ids << id;
        m_map->setLayoutProperty("annotations-layer", "filter",
            QVariantList{"in", QVariantList{"get", "id"},
                         QVariantList{"literal", ids}});
    }
}

void AnnotationManager::registerIcon(const QString& name, const QImage& image)
{
    if (image.isNull() || !m_map)
        return;

    m_iconRefCount[name]++;
    if (m_iconRefCount[name] == 1) {
        qreal dpr = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->devicePixelRatio() : 1.0;
        QImage scaled = image.scaledToWidth(static_cast<int>(image.width() * dpr), Qt::SmoothTransformation);
        m_map->removeImage(name);
        m_map->addImage(name, scaled);
        m_icons[name] = image;
    }
}

void AnnotationManager::unregisterIcon(const QString& name)
{
    if (!m_iconRefCount.contains(name))
        return;

    m_iconRefCount[name]--;
    if (m_iconRefCount[name] <= 0) {
        m_iconRefCount.remove(name);
        m_icons.remove(name);
        if (m_map)
            m_map->removeImage(name);
    }
}

void AnnotationManager::registerAllIcons(const QMap<QString, QImage>& icons)
{
    for (auto it = icons.begin(); it != icons.end(); ++it)
        registerIcon(it.key(), it.value());
}

void AnnotationManager::unregisterAllIcons()
{
    QStringList names;
    names.reserve(m_iconRefCount.size());
    for (auto it = m_iconRefCount.begin(); it != m_iconRefCount.end(); ++it)
        names.append(it.key());

    for (const auto& name : names)
        unregisterIcon(name);
}
