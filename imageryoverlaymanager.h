#ifndef IMAGERYOVERLAYMANAGER_H
#define IMAGERYOVERLAYMANAGER_H

#include <QObject>
#include <QString>

namespace QMapLibre { class Map; }

class ImageryOverlayManager : public QObject {
    Q_OBJECT

public:
    explicit ImageryOverlayManager(QMapLibre::Map* map, QObject* parent = nullptr);

    void setMapReady(bool ready);

    bool isVisible() const;
    void setVisible(bool visible);

    static QString tilesUrl();

    static constexpr const char* SOURCE_ID = "imagery";
    static constexpr const char* LAYER_ID = "satellite";

private:
    void ensureLayerSetup();

    QMapLibre::Map* m_map;
    bool m_ready = false;
    bool m_layerSetup = false;
    bool m_visible = true;
};

#endif
