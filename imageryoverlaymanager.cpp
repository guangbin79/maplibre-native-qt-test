#include "imageryoverlaymanager.h"
#include <QVector>
#include <QStringList>
#include <QVariantMap>
#include <QMapLibre/Map>

ImageryOverlayManager::ImageryOverlayManager(QMapLibre::Map* map, QObject* parent)
    : QObject(parent), m_map(map) {}

void ImageryOverlayManager::setMapReady(bool ready)
{
    m_ready = ready;
    ensureLayerSetup();
}

bool ImageryOverlayManager::isVisible() const
{
    return m_visible;
}

void ImageryOverlayManager::setVisible(bool visible)
{
    m_visible = visible;
    if (m_layerSetup && m_map) {
        m_map->setLayoutProperty(LAYER_ID, "visibility", visible ? "visible" : "none");
    }
}

QString ImageryOverlayManager::tilesUrl()
{
    return QStringLiteral("http://127.0.0.1:4943/gisserver/mbtiles/{z}/{x}/{y}/imagery");
}

void ImageryOverlayManager::ensureLayerSetup()
{
    if (m_layerSetup || !m_ready || !m_map) return;

    // z-order 不变量：mapReady 时无应用层⇒插顶部；后续应用层经各自锚定必落其上（annotationmanager 锚 location 之下、route/polygon 锚 annotations/location）——z-order 不依赖初始化时序
    QString before;
    const QVector<QString> existing = m_map->layerIds();
    const QStringList candidates = {"routes-solid", "polygons-fill", "annotations-layer", "location-indicator-layer"};
    for (const auto& id : existing) {
        if (candidates.contains(id)) {
            before = id;
            break;
        }
    }

    m_map->addSource("imagery", QVariantMap{
        {"type", "raster"},
        {"tiles", QStringList{tilesUrl()}},
        {"tileSize", 256},
        {"minzoom", 11},
        {"maxzoom", 14},
        {"attribution", "Contains modified Copernicus Sentinel data 2026"}
    });

    m_map->addLayer("satellite", QVariantMap{{"type", "raster"}, {"source", "imagery"}}, before);
    m_map->setPaintProperty("satellite", "raster-opacity", 0.7);
    m_map->setLayoutProperty("satellite", "visibility", m_visible ? "visible" : "none");

    m_layerSetup = true;
}
