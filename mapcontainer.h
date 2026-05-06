/**
 * @file mapcontainer.h
 * @brief 独立的 MapLibre 地图 Widget
 *
 * @section 概述
 * MapContainer 是一个跨平台地图渲染组件，基于 QMapLibre 构建，支持 Linux 和 Android 平台。
 * 提供完整的地图显示、交互控制和状态管理功能。
 *
 * @section 架构
 * - 继承自 QWidget，可嵌入任何 Qt 布局
 * - 内部封装 QMapLibre::GLWidget 负责 OpenGL 渲染
 * - 封装 QMapLibre::Map 提供底层地图操作接口
 * - 事件系统：重写 QWidget 事件处理，支持鼠标和触控
 *
 * @section 支持的交互
 * - 鼠标：左键拖拽平移、滚轮缩放
 * - 触控：单指平移、双指缩放/旋转
 *
 * @section 线程安全
 * 所有公共接口必须在主线程（UI 线程）中调用。
 * 内部渲染在独立的 GL 线程中进行，通过 Qt::QueuedConnection 与 UI 同步。
 *
 * @note 在桌面设备上，触控事件会被自动映射为鼠标事件处理。
 * @see QMapLibre::Map, QMapLibre::GLWidget
 */

#pragma once
#include <QWidget>
#include <QTouchEvent>
#include <QTimer>
#include <QMapLibre/Types>
#include "mapannotation.h"
#include "annotationmanager.h"
#include "maproutesegment.h"
#include "routemanager.h"
#include "locationindicatormanager.h"
#include "cameraanimationmath.h"
#include <QLabel>

namespace QMapLibre {
class GLWidget;
class Map;
}

/**
 * @brief 独立的 MapLibre 地图 Widget
 *
 * 跨平台地图渲染组件，支持 Linux 和 Android 平台。
 * 提供完整的地图显示、交互控制和状态管理功能。
 *
 * @section 功能特性
 * - 地图渲染：基于 OpenGL 的高性能矢量地图渲染
 * - 交互支持：鼠标拖拽、滚轮缩放、多点触控手势
 * - 状态管理：自动追踪地图中心、缩放、旋转、倾斜状态
 * - 信号通知：地图状态变化时发射 Qt 信号
 *
 * @section 使用示例
 * @code
 * // 基本使用
 * MapContainer::MapConfig config;
 * config.styleUrl = "https://demotiles.maplibre.org/style.json";
 * config.defaultCoordinate = QMapLibre::Coordinate(39.9, 116.4); // 北京
 * config.defaultZoom = 10.0;
 *
 * MapContainer *map = new MapContainer(config, this);
 * layout->addWidget(map);
 *
 * // 连接信号
 * connect(map, &MapContainer::zoomChanged, this, [](double zoom) {
 *     qDebug() << "Zoom level:" << zoom;
 * });
 *
 * // 动态控制
 * map->setCenter(31.2, 121.5); // 移动到上海
 * map->setZoom(12.0);
 * @endcode
 *
 * @section 注意事项
 * @warning 必须在主线程中创建和使用
 * @note 确保在使用前已加载有效的地图样式
 * @see MapConfig, setStyle(), setCenter(), setZoom()
 */
class MapContainer : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 地图初始化配置结构体
     *
     * 封装地图创建时所需的所有配置参数。
     * 提供合理的默认值，支持部分配置覆盖。
     *
     * @section 字段说明
     * - styleUrl: 地图样式定义文件的 URL，支持本地路径和网络 URL
     * - defaultCoordinate: 地图初始中心点坐标，格式为 (latitude, longitude)
     * - defaultZoom: 初始缩放级别，范围通常为 0-22
     *
     * @section 使用示例
     * @code
     * MapContainer::MapConfig config;
     * config.styleUrl = "https://example.com/style.json";
     * config.defaultCoordinate = QMapLibre::Coordinate(39.9, 116.4); // 北京
     * config.defaultZoom = 10.0;
     *
     * MapContainer *map = new MapContainer(config, parent);
     * @endcode
     *
     * @note 如果 styleUrl 为空，需要在构造后调用 setStyle() 设置样式
     * @warning 坐标顺序是 (lat, lon)，不是 (lon, lat)
     */
    struct MapConfig {
        QString styleUrl;                    ///< 地图样式 URL，指向 MapLibre 样式 JSON 文件
        QMapLibre::Coordinate defaultCoordinate; ///< 默认坐标 (latitude, longitude)，默认阿尔及尔
        double defaultZoom;                  ///< 默认缩放级别，范围 0-22，默认 8.0

        /**
         * @brief 构造配置对象
         * @param url 地图样式 URL，默认为空
         * @param coord 默认坐标，默认为阿尔及尔 (36.75, 3.05)
         * @param zoom 默认缩放级别，默认为 8.0
         */
        MapConfig(const QString &url = QString(),
                  const QMapLibre::Coordinate &coord = QMapLibre::Coordinate(36.75, 3.05),
                  double zoom = 8.0)
            : styleUrl(url), defaultCoordinate(coord), defaultZoom(zoom) {}
    };

    /**
     * @brief 构造地图容器
     *
     * 创建 MapContainer 实例，初始化内部 GLWidget 和 Map 对象。
     * 构造过程：
     * 1. 创建 QMapLibre::GLWidget 作为渲染表面
     * 2. 从 GLWidget 获取 QMapLibre::Map 实例
     * 3. 应用 MapConfig 中的配置（样式、坐标、缩放）
     * 4. 启用触控事件处理
     *
     * @param config 地图初始化配置，包含样式 URL、默认坐标和缩放级别
     * @param parent 父 Widget，用于内存管理和布局
     *
     * @section 使用场景
     * - 作为主窗口的中心地图组件
     * - 嵌入到复杂布局中的子地图视图
     * - 全屏地图应用的主容器
     *
     * @code
     * // 使用默认配置
     * MapContainer *map = new MapContainer(MapConfig(), this);
     *
     * // 使用自定义配置
     * MapContainer::MapConfig config;
     * config.styleUrl = "https://example.com/style.json";
     * MapContainer *map2 = new MapContainer(config, parent);
     * @endcode
     *
     * @note 如果 config.styleUrl 为空，地图将不会显示任何内容，
     *       需要后续调用 setStyle() 加载样式
     * @warning 必须在主线程中调用
     * @see MapConfig, setStyle()
     */
    explicit MapContainer(const MapConfig &config = MapConfig(), QWidget *parent = nullptr);

    /**
     * @brief 获取底层 QMapLibre::Map 实例
     *
     * 提供对底层 MapLibre 地图对象的直接访问，用于执行高级操作：
     * - 添加/移除图层和源
     * - 查询地图要素
     * - 设置滤镜和表达式
     * - 访问相机和动画 API
     *
     * @return Map 指针，可用于更底层的地图操作。生命周期与 MapContainer 相同。
     *
     * @section 使用场景
     * - 需要添加自定义图层（如热力图、轨迹线）
     * - 需要查询点击位置的地图要素
     * - 需要设置复杂的样式滤镜
     * - 需要执行相机飞行动画
     *
     * @code
     * QMapLibre::Map *map = container->map();
     * if (map) {
     *     // 添加 GeoJSON 源
     *     map->addSource("points", QMapLibre::GeoJSON({...}));
     *
     *     // 查询要素
     *     auto features = map->queryRenderedFeatures(screenPoint);
     * }
     * @endcode
     *
     * @note 返回的指针由 MapContainer 管理，不要手动 delete
     * @warning 在 MapContainer 销毁后，此指针将失效
     * @see QMapLibre::Map
     */
    QMapLibre::Map *map() const;

    /**
     * @brief 设置地图样式
     *
     * 加载并应用新的地图样式。样式定义了地图的外观，包括：
     * - 背景颜色和水体填充
     * - 道路、建筑、标签的显示规则
     * - 图层顺序和透明度
     * - 数据源配置
     *
     * @param styleUrl 样式 JSON URL，支持以下格式：
     *        - 网络 URL: "https://example.com/style.json"
     *        - 本地文件: "file:///path/to/style.json"
     *        - Qt 资源: ":/styles/default.json"
     *
     * @section 使用场景
     * - 切换日间/夜间主题
     * - 切换不同数据源的地图（卫星图、矢量图）
     * - 动态加载用户自定义样式
     *
     * @code
     * // 切换为卫星图样式
     * map->setStyle("https://demotiles.maplibre.org/style.json");
     *
     * // 使用本地样式
     * map->setStyle("file:///home/user/styles/custom.json");
     * @endcode
     *
     * @note 样式加载是异步的，地图会立即开始加载新样式
     * @warning 无效的 URL 会导致地图显示空白，但不会抛出异常
     * @see https://maplibre.org/maplibre-style-spec/
     */
    void setStyle(const QString &styleUrl);

    /**
     * @brief 设置地图中心点
     *
     * 将地图视图移动到指定的经纬度坐标，保持当前缩放级别不变。
     * 移动是立即生效的，没有动画效果。
     *
     * @param lat 纬度，范围 [-90, 90]
     *        - 正值：北半球
     *        - 负值：南半球
     * @param lon 经度，范围 [-180, 180]
     *        - 正值：东经
     *        - 负值：西经
     *
     * @section 使用场景
     * - 定位到用户当前位置
     * - 跳转到搜索结果位置
     * - 跟随移动目标（车辆、人员）
     *
     * @code
     * // 移动到北京
     * map->setCenter(39.9042, 116.4074);
     *
     * // 移动到纽约
     * map->setCenter(40.7128, -74.0060);
     * @endcode
     *
     * @note 坐标超出范围会被自动 clamp 到有效范围
     * @warning 坐标顺序是 (lat, lon)，与常见的 (lon, lat) GeoJSON 顺序不同
     * @see setZoom(), setBearing(), setPitch()
     */
    void setCenter(double lat, double lon);

    /**
     * @brief 设置地图缩放级别
     *
     * 控制地图的显示比例。缩放级别是指数级的，每增加 1 级，
     * 地图显示的细节增加一倍。
     *
     * @param zoom 缩放级别，典型范围：
     *        - 0:  全球视图
     *        - 5:  国家级别
     *        - 10: 城市级别
     *        - 15: 街道级别
     *        - 20: 建筑物级别
     *        实际范围取决于样式定义，通常为 0-22
     *
     * @section 使用场景
     * - 缩放到特定区域查看细节
     * - 根据数据密度自动调整视图
     * - 实现缩放按钮功能
     *
     * @code
     * // 缩放到城市级别
     * map->setZoom(10.0);
     *
     * // 缩放到街道级别
     * map->setZoom(15.5);
     * @endcode
     *
     * @note 缩放级别会被限制在样式支持的最小/最大范围内
     * @warning 非整数的缩放级别会产生平滑的过渡效果，但可能增加渲染负载
     * @see zoomChanged(), setCenter()
     */
    void setZoom(double zoom);

    /**
     * @brief 设置地图方位角（旋转角度）
     *
     * 控制地图的旋转方向。0 度表示正北朝上，顺时针增加。
     * 用于实现导航模式下的车头朝上显示。
     *
     * @param bearing 方位角，单位为度：
     *        - 0:   正北朝上（默认）
     *        - 90:  正东朝上
     *        - 180: 正南朝上
     *        - 270: 正西朝上
     *        值会自动归一化到 [0, 360) 范围
     *
     * @section 使用场景
     * - 导航应用中让地图跟随车头方向
     * - 根据设备指南针旋转地图
     * - 创建特殊的地图展示效果
     *
     * @code
     * // 设置正东朝上
     * map->setBearing(90.0);
     *
     * // 根据指南针角度旋转
     * map->setBearing(compassHeading);
     * @endcode
     *
     * @note 旋转会影响触控手势的坐标系，双指旋转手势会改变此值
     * @warning 在缩放级别较低时（如全球视图），旋转效果不明显
     * @see bearingChanged(), setPitch()
     */
    void setBearing(double bearing);

    /**
     * @brief 设置地图倾斜角度
     *
     * 控制地图的透视倾斜，模拟 3D 视角。0 度为垂直俯视，
     * 增加倾斜角度可以看到建筑物的侧面。
     *
     * @param pitch 倾斜角度，单位为度：
     *        - 0:   垂直俯视（默认）
     *        - 30:  轻微倾斜，适合一般浏览
     *        - 60:  明显倾斜，适合展示 3D 建筑
     *        最大有效值取决于样式和渲染器，通常为 60-85 度
     *
     * @section 使用场景
     * - 展示 3D 建筑物效果
     * - 创建更真实的地图视角
     * - 导航应用中的透视效果
     *
     * @code
     * // 轻微倾斜，适合浏览
     * map->setPitch(30.0);
     *
     * // 最大倾斜，展示 3D 效果
     * map->setPitch(60.0);
     * @endcode
     *
     * @note 倾斜角度过大可能导致远处地图细节丢失
     * @warning 需要样式支持 3D 图层（如 fill-extrusion）才能看到建筑物立体效果
     * @see tiltChanged(), setBearing()
     */
    void setPitch(double pitch);

    // ===== 相机动画接口 =====

    /**
     * @brief 相机动画过渡（平滑飞到目标位置）
     *
     * 同时过渡 center/zoom/bearing/pitch 到目标值，使用 ease-in-out 缓动曲线。
     * 约 60fps 帧率，bearing 自动选择最短旋转方向。
     *
     * 行为规则：
     * - 新调用替换正在进行的动画（从中间位置开始）
     * - 用户触摸/拖拽地图时自动取消动画
     * - 目标值 == 当前值时立即 emit animationFinished()
     * - bearing 350° → 10° 走正向 +20°（不走 -340°）
     * - zoom 自动 clamp [0, 18]，pitch 自动 clamp [0°, 60°]
     *
     * @param lat       目标纬度 [-90, 90]
     * @param lon       目标经度 [-180, 180]
     * @param zoom      目标缩放级别 [0, 18]
     * @param bearing   目标朝向角度 [0, 360)
     * @param pitch     目标倾斜角度 [0, 60]
     * @param durationMs 动画时长（毫秒），-1 表示使用全局默认值
     *
     * @code
     * // 使用默认时长飞到北京
     * mapContainer->animateTo(39.9042, 116.4074, 15.0, 0.0, 45.0);
     *
     * // 自定义 2 秒动画
     * mapContainer->animateTo(31.23, 121.47, 12.0, 0.0, 0.0, 2000);
     *
     * // 修改全局默认时长为 1 秒
     * mapContainer->setDefaultAnimationDuration(1000);
     * @endcode
     *
     * @see setDefaultAnimationDuration(), animationFinished(), setCenter()
     */
    void animateTo(double lat, double lon, double zoom, double bearing, double pitch, int durationMs = -1);

    /**
     * @brief 设置相机动画的默认时长
     *
     * 设置后，animateTo() 的 durationMs 参数传 -1 时使用此值。
     *
     * @param ms 默认动画时长（毫秒），默认 500ms
     *
     * @see animateTo()
     */
    void setDefaultAnimationDuration(int ms);

    // ===== 标注管理接口 =====

    /**
     * @brief 批量设置标注（替换全部）
     *
     * 清除所有现有标注，用新的列表替换。同时注册所有需要的图标。
     *
     * @param annotations 标注列表，每个包含 id/latitude/longitude/title/iconName
     * @param icons       图标映射表，key 对应 annotation.iconName，value 为 QImage
     *
     * @code
     * QMap<QString, QImage> icons;
     * icons["marker"] = QImage(":/icons/marker.png");
     * icons["shop"] = QImage(":/icons/shop.png");
     *
     * QVector<MapAnnotation> anns = {
     *     { .id="p1", .latitude=39.9, .longitude=116.4, .title="天安门", .iconName="marker" },
     *     { .id="p2", .latitude=31.2, .longitude=121.5, .title="外滩", .iconName="shop" }
     * };
     * mapContainer->setAnnotations(anns, icons);
     * @endcode
     *
     * @see addAnnotation(), removeAnnotation(), clearAnnotations()
     */
    void setAnnotations(const QVector<MapAnnotation>& annotations,
                        const QMap<QString, QImage>& icons);

    /**
     * @brief 清除全部标注和图标
     *
     * 移除所有标注及其关联的图标资源。
     *
     * @see setAnnotations()
     */
    void clearAnnotations();

    /**
     * @brief 添加单个标注
     *
     * @param annotation 标注数据
     * @param icon       标注图标（如果 iconName 对应的图标尚未注册，需传入）
     *
     * @code
     * MapAnnotation ann;
     * ann.id = "poi-001";
     * ann.latitude = 39.9042;
     * ann.longitude = 116.4074;
     * ann.title = "北京天安门";
     * ann.iconName = "marker";
     * mapContainer->addAnnotation(ann, QImage(":/icons/marker.png"));
     * @endcode
     *
     * @see addAnnotations(), setAnnotations()
     */
    void addAnnotation(const MapAnnotation& annotation,
                       const QImage& icon = QImage());

    /**
     * @brief 批量添加标注
     *
     * @param annotations 标注列表
     * @param icons       需要注册的图标映射表
     *
     * @see addAnnotation(), setAnnotations()
     */
    void addAnnotations(const QVector<MapAnnotation>& annotations,
                        const QMap<QString, QImage>& icons);

    /**
     * @brief 移除单个标注（按ID）
     *
     * @param id 标注唯一标识
     *
     * @see removeAnnotations(), clearAnnotations()
     */
    void removeAnnotation(const QString& id);

    /**
     * @brief 批量移除标注（按ID列表）
     *
     * @param ids 标注ID列表
     *
     * @see removeAnnotation(), clearAnnotations()
     */
    void removeAnnotations(const QStringList& ids);

    /**
     * @brief 设置可见标注（按ID白名单）
     *
     * 只有列表中的标注可见，其余隐藏。底层使用 MapLibre filter 表达式（~0.1ms）。
     *
     * @param ids 可见标注ID列表
     *
     * @code
     * // 只显示这两个标注
     * mapContainer->setVisibleIds({"poi-001", "poi-002"});
     * @endcode
     *
     * @see showAllAnnotations(), hideAllAnnotations()
     */
    void setVisibleIds(const QStringList& ids);

    /**
     * @brief 显示全部标注
     * @see hideAllAnnotations(), setVisibleIds()
     */
    void showAllAnnotations();

    /**
     * @brief 隐藏全部标注
     * @see showAllAnnotations(), setVisibleIds()
     */
    void hideAllAnnotations();

    /**
     * @brief 获取所有标注ID列表
     * @return 所有已添加标注的ID
     * @see visibleIds()
     */
    QStringList allIds() const;

    /**
     * @brief 获取当前可见标注ID列表
     * @return 当前可见标注的ID
     * @see allIds(), setVisibleIds()
     */
    QStringList visibleIds() const;

    // ===== 线路管理接口 =====

    /**
     * @brief 批量设置线路段（替换全部）
     *
     * 清除所有现有线路段，用新的列表替换。
     * 多个段共享相同 routeId 会被视为同一条逻辑线路。
     *
     * @param segments 线路段列表
     *
     * @code
     * QVector<MapRouteSegment> segs = {
     *     { .id="s1", .routeId="route-A", .coordinates={{39.9,116.4},{39.92,116.38}},
     *       .color=QColor("#FF5722"), .width=4.0, .dashed=false },
     *     { .id="s2", .routeId="route-A", .coordinates={{39.92,116.38},{39.94,116.36}},
     *       .color=QColor("#FF5722"), .width=4.0, .dashed=true }
     * };
     * mapContainer->setRoutes(segs);
     * @endcode
     *
     * @see addRouteSegment(), clearRoutes(), MapRouteSegment
     */
    void setRoutes(const QVector<MapRouteSegment>& segments);

    /** @brief 清除全部线路段 @see setRoutes() */
    void clearRoutes();

    /**
     * @brief 添加单个线路段
     * @param segment 线路段数据
     * @see addRouteSegments(), setRoutes()
     */
    void addRouteSegment(const MapRouteSegment& segment);

    /**
     * @brief 批量添加线路段
     * @param segments 线路段列表
     * @see addRouteSegment(), setRoutes()
     */
    void addRouteSegments(const QVector<MapRouteSegment>& segments);

    /**
     * @brief 移除单个线路段（按段ID）
     * @param id 线路段唯一标识
     * @see removeRouteSegments(), clearRoutes()
     */
    void removeRouteSegment(const QString& id);

    /**
     * @brief 批量移除线路段（按段ID列表）
     * @param ids 线路段ID列表
     * @see removeRouteSegment(), clearRoutes()
     */
    void removeRouteSegments(const QStringList& ids);

    /**
     * @brief 设置可见线路（按线路ID白名单）
     *
     * 只有列表中 routeId 对应的线路段可见。底层使用 compound filter（~0.1ms）。
     *
     * @param routeIds 可见线路ID列表
     *
     * @code
     * mapContainer->setVisibleRouteIds({"route-A", "route-B"});
     * @endcode
     *
     * @see showAllRoutes(), hideAllRoutes()
     */
    void setVisibleRouteIds(const QStringList& routeIds);

    /** @brief 显示全部线路 @see hideAllRoutes(), setVisibleRouteIds() */
    void showAllRoutes();

    /** @brief 隐藏全部线路 @see showAllRoutes(), setVisibleRouteIds() */
    void hideAllRoutes();

    /**
     * @brief 获取所有线路ID列表（去重后的 routeId）
     * @return 所有不重复的 routeId
     * @see visibleRouteIds()
     */
    QStringList allRouteIds() const;

    /**
     * @brief 获取当前可见线路ID列表
     * @return 当前可见线路的 routeId
     * @see allRouteIds(), setVisibleRouteIds()
     */
    QStringList visibleRouteIds() const;

    // ===== 位置指示器接口 =====

    /**
     * @brief 设置当前位置坐标
     *
     * 更新位置指示器的 GPS 坐标。
     * - Free 模式：图标在地图上移到新坐标
     * - Fixed 模式：地图平移使该坐标对准屏幕固定点
     *
     * @param lat 纬度 [-90, 90]
     * @param lon 经度 [-180, 180]
     *
     * @see setLocationMode(), showLocation()
     */
    void setLocation(double lat, double lon);

    /**
     * @brief 设置位置指示器图标
     *
     * @param icon 图标图片，建议使用正方形 PNG（带透明通道）
     *
     * @code
     * mapContainer->setLocationIcon(QImage(":/icons/location_arrow.png"));
     * @endcode
     */
    void setLocationIcon(const QImage& icon);

    /**
     * @brief 设置位置指示器旋转角度
     *
     * 旋转图标，用于导航场景指示行进方向。
     * - Free 模式：通过 MapLibre icon-rotate 属性旋转
     * - Fixed 模式：通过 QTransform 旋转 overlay 图标
     *
     * @param degrees 旋转角度（度），0 表示图标朝上（北），顺时针增加
     *
     * @code
     * // 设置图标指向东方（90度）
     * mapContainer->setLocationRotation(90.0);
     *
     * // 根据 GPS 方向旋转
     * mapContainer->setLocationRotation(gpsCourse);
     * @endcode
     *
     * @see setLocationIcon(), locationRotation()
     */
    void setLocationRotation(double degrees);

    /**
     * @brief 获取当前位置指示器旋转角度
     * @return 当前旋转角度（度）
     * @see setLocationRotation()
     */
    double locationRotation() const;

    /**
     * @brief 设置位置指示器模式
     *
     * - Free：图标渲染在地图坐标上，随地图移动（浏览模式）
     * - Fixed：图标固定在屏幕位置，地图跟随移动（导航模式）
     *
     * @param mode LocationMode::Free 或 LocationMode::Fixed
     *
     * @code
     * // 切换到导航模式
     * mapContainer->setLocationMode(LocationIndicatorManager::Fixed);
     * // 切回浏览模式
     * mapContainer->setLocationMode(LocationIndicatorManager::Free);
     * @endcode
     *
     * @see LocationIndicatorManager::LocationMode
     */
    void setLocationMode(LocationIndicatorManager::LocationMode mode);

    /**
     * @brief 获取当前位置指示器模式
     * @return 当前 LocationMode
     * @see setLocationMode()
     */
    LocationIndicatorManager::LocationMode locationMode() const;

    /**
     * @brief 显示位置指示器
     * @see hideLocation(), isLocationVisible()
     */
    void showLocation();

    /**
     * @brief 隐藏位置指示器
     * @see showLocation(), isLocationVisible()
     */
    void hideLocation();

    /**
     * @brief 查询位置指示器是否可见
     * @return true 可见，false 隐藏
     */
    bool isLocationVisible() const;

    /**
     * @brief 设置 Fixed 模式的中心偏移量
     *
     * 将地图可视中心从视口正中心向下偏移指定像素数。
     * 仅 Fixed 模式生效。
     *
     * @param bottomPixels 从视口底部向上的偏移像素数（如 200 表示中心在底部上方 200px）
     *
     * @code
     * // 将中心点移到屏幕下方 1/3 位置（假设窗口高度 600px）
     * mapContainer->setCenterOffset(200);
     * @endcode
     *
     * @see setLocationMode()
     */
    void setCenterOffset(int bottomPixels);

    /**
     * @brief 设置 Fixed 模式下是否允许触屏滑动地图
     *
     * 启用后，Fixed 模式下用户可触屏平移地图，地图跟随暂停；
     * 松手后经过 setFixedTouchResumeTimeout() 设定的超时时间，
     * 自动恢复 Fixed 模式并平滑飞回最新 GPS 位置。
     * 禁用时，Fixed 模式下触屏滑动无效。
     *
     * @param enabled true 允许触屏滑动，false 禁止（默认 false）
     *
     * @code
     * mapContainer->setFixedTouchPanEnabled(true);
     * mapContainer->setFixedTouchResumeTimeout(3000);  // 3 秒后恢复
     * @endcode
     *
     * @see isFixedTouchPanEnabled(), setFixedTouchResumeTimeout()
     */
    void setFixedTouchPanEnabled(bool enabled);

    /**
     * @brief 查询 Fixed 模式下是否允许触屏滑动地图
     * @return true 允许，false 禁止
     * @see setFixedTouchPanEnabled()
     */
    bool isFixedTouchPanEnabled() const;

    /**
     * @brief 设置触屏滑动后恢复 Fixed 模式的超时时间
     *
     * 用户松手后经过此时间（毫秒），自动从 Free 切回 Fixed 模式，
     * 并通过 animateTo 平滑飞回最新 GPS 位置。
     *
     * @param ms 超时时间（毫秒），默认 3000
     *
     * @see fixedTouchResumeTimeout(), setFixedTouchPanEnabled()
     */
    void setFixedTouchResumeTimeout(int ms);

    /**
     * @brief 获取触屏滑动后恢复 Fixed 模式的超时时间
     * @return 超时时间（毫秒）
     * @see setFixedTouchResumeTimeout()
     */
    int fixedTouchResumeTimeout() const;
    bool isMapReady() const { return m_mapReady; }

signals:
    /**
     * @brief 缩放级别变化信号
     *
     * 当地图缩放级别发生变化时发射。触发时机包括：
     * - 用户滚轮缩放
     * - 用户双指捏合缩放
     * - 调用 setZoom() 方法
     * - 地图加载完成后的初始状态
     *
     * @param zoom 当前缩放级别，范围取决于样式定义（通常为 0-22）
     *
     * @section 连接示例
     * @code
     * // 使用 Lambda 表达式
     * connect(map, &MapContainer::zoomChanged, this, [](double zoom) {
     *     qDebug() << "当前缩放级别:" << zoom;
     *     zoomLabel->setText(QString("Zoom: %1").arg(zoom, 0, 'f', 1));
     * });
     *
     * // 连接到成员函数
     * connect(map, &MapContainer::zoomChanged,
     *         this, &MainWindow::onZoomChanged);
     * @endcode
     *
     * @section 使用场景
     * - 更新 UI 上的缩放级别显示
     * - 根据缩放级别动态加载/卸载数据
     * - 实现缩放按钮的启用/禁用状态
     * - 记录用户交互日志
     *
     * @note 信号可能在地图渲染线程中发射，槽函数中避免耗时操作
     * @warning 快速连续缩放时，信号会频繁发射，注意性能影响
     * @see setZoom(), setCenter()
     */
    void zoomChanged(double zoom);

    /**
     * @brief 方位角变化信号
     *
     * 当地图旋转角度发生变化时发射。触发时机包括：
     * - 用户双指旋转手势
     * - 调用 setBearing() 方法
     * - 地图复位操作
     *
     * @param bearing 当前方位角，单位为度，范围 [0, 360)
     *        - 0:   正北朝上
     *        - 90:  正东朝上
     *        - 180: 正南朝上
     *        - 270: 正西朝上
     *
     * @section 连接示例
     * @code
     * connect(map, &MapContainer::bearingChanged, this, [this](double bearing) {
     *     // 更新指南针 UI
     *     compassWidget->setRotation(bearing);
     *
     *     // 显示方向文字
     *     QString direction;
     *     if (bearing >= 315 || bearing < 45) direction = "北";
     *     else if (bearing >= 45 && bearing < 135) direction = "东";
     *     else if (bearing >= 135 && bearing < 225) direction = "南";
     *     else direction = "西";
     *     directionLabel->setText(direction);
     * });
     * @endcode
     *
     * @section 使用场景
     * - 更新指南针控件
     * - 导航应用中同步车头方向
     * - 显示当前朝向文字
     *
     * @note 值会自动归一化到 [0, 360) 范围
     * @warning 在缩放级别较低时，用户可能不会注意到旋转效果
     * @see setBearing(), setPitch()
     */
    void bearingChanged(double bearing);

    /**
     * @brief 倾斜角度变化信号
     *
     * 当地图倾斜角度发生变化时发射。触发时机包括：
     * - 用户双指上下滑动（在支持倾斜的平台上）
     * - 调用 setPitch() 方法
     * - 地图复位操作
     *
     * @param tilt 当前倾斜角度，单位为度，范围通常为 [0, 60] 或 [0, 85]
     *        - 0:   垂直俯视
     *        - 30:  轻微倾斜
     *        - 60:  明显倾斜，3D 效果明显
     *
     * @section 连接示例
     * @code
     * connect(map, &MapContainer::tiltChanged, this, [this](double tilt) {
     *     // 更新倾斜滑块位置
     *     tiltSlider->setValue(static_cast<int>(tilt));
     *
     *     // 根据倾斜角度调整 UI
     *     if (tilt > 30) {
     *         buildingToggle->setEnabled(true);
     *     }
     * });
     * @endcode
     *
     * @section 使用场景
     * - 同步倾斜控制滑块
     * - 根据倾斜角度启用/禁用 3D 功能
     * - 实现倾斜复位按钮
     *
     * @note 不是所有平台都支持触控手势改变倾斜角度
     * @warning 倾斜角度过大时，远处地图细节可能丢失
     * @see setPitch(), setBearing()
     */
    void tiltChanged(double tilt);

    /**
     * @brief 触摸手势开始信号
     *
     * 当检测到触摸按下（TouchBegin）时发射，用于通知外部控件
     * 暂时禁用更新以减少 CPU 竞争，提升地图渲染性能。
     */
    void touchBegin();

    /**
     * @brief 触摸手势结束信号
     *
     * 当触摸释放（TouchEnd/TouchCancel）时发射，用于通知外部控件
     * 恢复正常的更新和重绘。
     */
    void touchEnd();

    /**
     * @brief 中心点变化信号
     *
     * 当地图中心点坐标发生变化时发射。触发时机包括：
     * - 用户拖拽平移
     * - 用户单指滑动
     * - 调用 setCenter() 方法
     * - 惯性滚动动画期间
     *
     * @param lat 当前中心点纬度，范围 [-90, 90]
     * @param lon 当前中心点经度，范围 [-180, 180]
     *
     * @section 连接示例
     * @code
     * connect(map, &MapContainer::centerChanged, this, [this](double lat, double lon) {
     *     // 更新坐标显示
     *     coordLabel->setText(QString("%1, %2")
     *         .arg(lat, 0, 'f', 4)
     *         .arg(lon, 0, 'f', 4));
     *
     *     // 检查是否在目标区域内
     *     if (isInTargetArea(lat, lon)) {
     *         showTargetReachedMessage();
     *     }
     * });
     * @endcode
     *
     * @section 使用场景
     * - 实时显示地图中心坐标
     * - 地理围栏检测
     * - 动态加载视口内的数据
     * - 记录用户浏览轨迹
     *
     * @note 拖拽和惯性滚动期间会频繁发射此信号
     * @warning 坐标顺序是 (lat, lon)，注意与 GeoJSON (lon, lat) 的区别
     * @see setCenter(), zoomChanged()
     */
    void centerChanged(double lat, double lon);

    /**
     * @brief 相机动画完成信号
     *
     * 当 animateTo() 动画播放完毕时触发。
     * 目标值 == 当前值时也会立即触发。
     *
     * @code
     * connect(mapContainer, &MapContainer::animationFinished, []() {
     *     qDebug() << "动画结束";
     * });
     * @endcode
     *
     * @see animateTo()
     */
    void animationFinished();
    void mapReady();

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QMapLibre::GLWidget *m_glWidget;
    double m_lastZoom = 0.0;
    double m_lastBearing = 0.0;
    double m_lastPitch = 0.0;
    double m_lastLat = 0.0;
    double m_lastLon = 0.0;

    bool m_touchActive = false;
    int m_touchPointCount = 0;
    QList<QTouchEvent::TouchPoint> m_lastTouchPoints;
    QPointF m_lastMousePos;

    // 复合手势状态识别：锁定主导手势，避免缩放/旋转/倾斜互相干扰
    enum class GestureMode { None, Scale, Rotate, Pitch, Both };
    GestureMode m_gestureMode = GestureMode::None;
    qreal m_initialPinchDist = 0.0;    ///< 双指按下时的初始距离
    qreal m_initialPinchAngle = 0.0;   ///< 双指按下时的初始角度
    QPointF m_initialPinchCenter;      ///< 双指按下时的初始中心点

    // 旋转累积与节流：降低渲染频率避免 zoom 8 卡顿
    qreal m_accumulatedRotation = 0.0; ///< 累积的角度变化
    int m_rotationSkipCounter = 0;     ///< 旋转帧跳过计数器
    int m_panSkipCounter = 0;          ///< 平移帧跳过计数器

    // 倾斜手势参数
    static constexpr double MIN_PITCH = 0.0;   ///< 最小倾斜角度
    static constexpr double MAX_PITCH = 60.0;  ///< 最大倾斜角度
    static constexpr qreal PITCH_SENSITIVITY = 0.15; ///< 倾斜灵敏度系数

    // 双击检测：用于单指双击放大地图
    qint64 m_lastTouchEndTime = 0;     ///< 上次触摸结束的时间戳（毫秒）
    QPointF m_lastTouchEndPos;         ///< 上次触摸结束的位置
    static constexpr qint64 DOUBLE_TAP_INTERVAL_MS = 300;  ///< 双击时间阈值（毫秒）
    static constexpr qreal DOUBLE_TAP_DISTANCE_PX = 50.0;  ///< 双击距离阈值（像素）
    static constexpr double MAX_ZOOM = 18.0;  ///< 地图最大缩放级别

    // 双指点击缩小检测
    qint64 m_twoFingerTapStartTime = 0;    ///< 双指按下时的起始时间戳（第一指的 pressTimestamp）
    QPointF m_twoFingerTapStartPos1;        ///< 双指按下时第一指位置
    QPointF m_twoFingerTapStartPos2;        ///< 双指按下时第二指位置
    qreal m_twoFingerTapInitialDist = 0.0;  ///< 双指按下时两指距离
    static constexpr qint64 TWO_FINGER_TAP_DURATION_MS = 300;   ///< 双指点击最大持续时间（毫秒）
    static constexpr qreal TWO_FINGER_TAP_MAX_DRIFT_PX = 30.0;  ///< 双指点击最大手指漂移距离（像素）
    static constexpr qreal TWO_FINGER_TAP_DIST_CHANGE_RATIO = 0.15; ///< 双指点击最大距离变化比例（15%）

    // 双击放大
    QPointF m_doubleTapAnimCenter;            ///< 双击放大的中心点

    QTimer* m_cameraAnimTimer = nullptr;
    int m_cameraAnimStep = 0;
    int m_cameraAnimTotalSteps = 0;
    double m_animStartLat = 0.0;
    double m_animStartLon = 0.0;
    double m_animStartZoom = 0.0;
    double m_animStartBearing = 0.0;
    double m_animStartPitch = 0.0;
    double m_animTargetLat = 0.0;
    double m_animTargetLon = 0.0;
    double m_animTargetZoom = 0.0;
    double m_animTargetBearing = 0.0;
    double m_animTargetPitch = 0.0;
    int m_defaultAnimDuration = 500;

    QTimer* m_followTimer = nullptr;
    double m_followTargetLat = 0.0;
    double m_followTargetLon = 0.0;
    static constexpr double FOLLOW_LERP_FACTOR = 0.15;
    static constexpr int LOCATION_OVERLAY_SIZE = 40;  ///< 位置覆盖图标尺寸（像素）

    bool m_fixedTouchPanEnabled = false;
    int m_fixedTouchResumeTimeout = 3000;
    QTimer* m_fixedResumeTimer = nullptr;
    bool m_fixedPausedByTouch = false;

    void stopCameraAnimation();

    AnnotationManager* m_annotationManager = nullptr;
    RouteManager* m_routeManager = nullptr;
    LocationIndicatorManager* m_locationIndicatorManager = nullptr;
    QLabel* m_locationOverlay = nullptr;
    bool m_mapReady = false;

    void connectMapSignals();

private slots:
    void onCameraAnimStep();
    void onFollowStep();
};

