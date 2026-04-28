#ifndef ANNOTATIONMANAGER_H
#define ANNOTATIONMANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QImage>
#include "mapannotation.h"

namespace QMapLibre { class Map; }

class AnnotationManager : public QObject {
    Q_OBJECT

public:
    explicit AnnotationManager(QMapLibre::Map* map, QObject* parent = nullptr);

    void setMapReady(bool ready);

    void setAnnotations(const QVector<MapAnnotation>& annotations,
                        const QMap<QString, QImage>& icons);
    void clearAnnotations();

    void addAnnotation(const MapAnnotation& annotation,
                       const QImage& icon = QImage());
    void addAnnotations(const QVector<MapAnnotation>& annotations,
                        const QMap<QString, QImage>& icons);
    void removeAnnotation(const QString& id);
    void removeAnnotations(const QStringList& ids);

    void setVisibleIds(const QStringList& ids);
    void showAllAnnotations();
    void hideAllAnnotations();

    QStringList allIds() const;
    QStringList visibleIds() const;

private:
    void ensureLayerSetup();
    void rebuildSource();
    void updateFilter();

    void registerIcon(const QString& name, const QImage& image);
    void unregisterIcon(const QString& name);
    void registerAllIcons(const QMap<QString, QImage>& icons);
    void unregisterAllIcons();

    QMapLibre::Map* m_map;
    QVector<MapAnnotation> m_annotations;
    QMap<QString, QImage> m_icons;
    QMap<QString, int> m_iconRefCount;
    QStringList m_visibleIds;
    bool m_ready = false;
    bool m_layerSetup = false;
};

#endif
