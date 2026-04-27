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
#include <QMapLibre/Types>

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

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

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

    // 复合手势状态识别：锁定主导手势，避免缩放/旋转互相干扰
    enum class GestureMode { None, Scale, Rotate, Both };
    GestureMode m_gestureMode = GestureMode::None;
    qreal m_initialPinchDist = 0.0;    ///< 双指按下时的初始距离
    qreal m_initialPinchAngle = 0.0;   ///< 双指按下时的初始角度

    // 旋转累积与节流：降低渲染频率避免 zoom 8 卡顿
    qreal m_accumulatedRotation = 0.0; ///< 累积的角度变化
    int m_rotationSkipCounter = 0;     ///< 帧跳过计数器
};
