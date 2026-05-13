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

    /**
     * @brief 批量设置标注（替换全部）
     *
     * 清除所有现有标注，用新的列表替换。
     * 图标需事先通过 registerIcon()/registerAllIcons() 注册，否则对应标注会显示空白图标。
     *
     * @param annotations 标注列表，每个包含 id/latitude/longitude/title/iconName
     *
     * @code
     * QVector<MapAnnotation> anns = {
     *     { .id="p1", .latitude=39.9, .longitude=116.4, .title="天安门", .iconName="marker" }
     * };
     * manager->registerIcon("marker", QImage(":/icons/marker.png"));
     * manager->setAnnotations(anns);
     * @endcode
     *
     * @see addAnnotation(), clearAnnotations(), registerIcon()
     */
    void setAnnotations(const QVector<MapAnnotation>& annotations);

    /** @brief 清除全部标注 @see setAnnotations() */
    void clearAnnotations();

    /**
     * @brief 添加单个标注
     *
     * 图标需事先通过 registerIcon()/registerAllIcons() 注册。
     * 如果 iconName 对应的图标未注册，该标注会显示空白图标。
     *
     * @param annotation 标注数据
     *
     * @code
     * MapAnnotation ann;
     * ann.id = "poi-001";
     * ann.latitude = 39.9042;
     * ann.longitude = 116.4074;
     * ann.title = "天安门";
     * ann.iconName = "marker";
     * manager->addAnnotation(ann);
     * @endcode
     *
     * @see addAnnotations(), setAnnotations(), registerIcon()
     */
    void addAnnotation(const MapAnnotation& annotation);

    /**
     * @brief 批量添加标注
     *
     * 图标需事先通过 registerIcon()/registerAllIcons() 注册。
     *
     * @param annotations 标注列表
     *
     * @see addAnnotation(), setAnnotations(), registerAllIcons()
     */
    void addAnnotations(const QVector<MapAnnotation>& annotations);

    /** @brief 移除单个标注（按ID） @param id 标注唯一标识 @see removeAnnotations(), clearAnnotations() */
    void removeAnnotation(const QString& id);

    /** @brief 批量移除标注（按ID列表） @param ids 标注ID列表 @see removeAnnotation(), clearAnnotations() */
    void removeAnnotations(const QStringList& ids);

    void setVisibleIds(const QStringList& ids);
    void showAllAnnotations();
    void hideAllAnnotations();

    QStringList allIds() const;
    QStringList visibleIds() const;

    /**
     * @brief 获取所有标注数据
     *
     * 返回当前管理器中所有标注的副本。可用于导出、序列化或遍历查询。
     *
     * @return 标注数据的 QVector 副本
     *
     * @code
     * auto anns = manager->annotations();
     * QByteArray geojson = GeoJsonExporter::buildAnnotations(anns);
     * @endcode
     *
     * @see setAnnotations(), GeoJsonExporter::buildAnnotations()
     */
    QVector<MapAnnotation> annotations() const;

public:
    /**
     * @brief 注册单个标注图标
     *
     * 在添加标注前注册图标。同一图标可多次注册（幂等）。
     * 图标名称对应 MapAnnotation::iconName 字段。
     *
     * @param name  图标名称，对应 annotation.iconName
     * @param image 图标图片，建议使用正方形 PNG（带透明通道）
     *
     * @code
     * manager->registerIcon("marker", QImage(":/icons/marker.png"));
     * manager->registerIcon("shop", QImage(":/icons/shop.png"));
     * @endcode
     *
     * @see registerAllIcons(), unregisterIcon()
     */
    void registerIcon(const QString& name, const QImage& image);

    /** @brief 注销单个标注图标 @param name 图标名称 @see registerIcon() */
    void unregisterIcon(const QString& name);

    /**
     * @brief 批量注册标注图标
     *
     * @param icons 图标映射表，key 对应 annotation.iconName，value 为 QImage
     *
     * @code
     * QMap<QString, QImage> icons;
     * icons["marker"] = QImage(":/icons/marker.png");
     * icons["shop"] = QImage(":/icons/shop.png");
     * manager->registerAllIcons(icons);
     * @endcode
     *
     * @see registerIcon(), unregisterAllIcons()
     */
    void registerAllIcons(const QMap<QString, QImage>& icons);

    /** @brief 注销全部标注图标 @see registerAllIcons() */
    void unregisterAllIcons();

private:
    void ensureLayerSetup();
    void rebuildSource();
    void updateFilter();

    QMapLibre::Map* m_map;
    QVector<MapAnnotation> m_annotations;
    QMap<QString, QImage> m_icons;
    QMap<QString, int> m_iconRefCount;
    QStringList m_visibleIds;
    bool m_ready = false;
    bool m_layerSetup = false;
};

#endif
