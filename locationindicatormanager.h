/**
 * @file locationindicatormanager.h
 * @brief 位置指示器管理器
 *
 * 管理当前位置图标在地图上的渲染，支持两种互斥模式：
 *
 * - Free（自由定位）：图标渲染在地图上的实际 GPS 坐标位置，
 *   使用 Symbol Layer + GeoJSON 实现。随地图平移/缩放而变化。
 *   适合普通浏览场景。
 *
 * - Fixed（固定中心）：图标通过 setMargins 偏移地图可视中心，
 *   渲染在屏幕固定位置（如底部 1/3 处），地图跟随 GPS 平移。
 *   同样使用 Symbol Layer 实现，图标始终锚定在 GPS 坐标上，
 *   缩放/旋转/倾斜时位置精确。
 *   适合导航场景。
 *
 * 状态机：Hidden → FreeVisible / FixedFollowing → FixedBrowsing（用户拖拽）
 * 导航朝向：HeadingUp（地图旋转跟随航向）/ NorthUp（地图保持正北，图标旋转）
 */


#ifndef LOCATIONINDICATORMANAGER_H
#define LOCATIONINDICATORMANAGER_H

#include <QObject>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <optional>

namespace QMapLibre { class Map; }
class MapContainer;

/**
 * @brief 位置指示器管理器
 *
 * 负责管理当前位置图标在地图上的渲染方式和交互行为。
 *
 * 架构：Free 和 Fixed 模式均使用 Symbol Layer 渲染图标。
 * Free 模式下图标跟随 GPS 坐标在地图上移动；
 * Fixed 模式下通过 setMargins 偏移地图中心，使 GPS 坐标的图标
 * 显示在屏幕固定位置。
 *
 * 状态机（State）：
 * - Hidden：隐藏
 * - FreeVisible：Free 模式可见
 * - FixedFollowing：Fixed 模式跟随 GPS
 * - FixedBrowsing：Fixed 模式用户拖拽中（自动检测）
 *
 * 朝向模式（HeadingMode，仅 Fixed 模式）：
 * - HeadingUp：地图旋转跟随航向，图标保持屏幕垂直
 * - NorthUp：地图保持正北，图标旋转指向航向
 *
 * @see LocationMode, HeadingMode, State
 */
class LocationIndicatorManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 位置指示器显示模式
     */
    enum class LocationMode {
        Free,    ///< 自由定位 — 图标渲染在地图坐标上，随地图移动
        Fixed    ///< 固定中心 — 图标固定在屏幕位置，地图跟随移动
    };

    /**
     * @brief 导航朝向显示策略（仅在 Fixed 模式下生效）
     */
    enum class FixedHeadingMode {
        HeadingUp, ///< 车头朝上 — 地图随导航方向旋转，屏幕图标保持垂直向上
        NorthUp    ///< 正北朝上 — 地图保持正北，屏幕图标自身旋转指向航向
    };

    /**
     * @brief 构造位置指示器管理器
     *
     * @param map    QMapLibre::Map 实例指针，用于:
     *   1. 监控地图 Ready 信号
     *   2. 操作地图图层和源
     *   3. Fixed 的 HeadingUp 下，操作地图旋转
     *   4. Fixed 下，驶入驶出路口，地图比例尺缩放
     * @param parent 父对象，用于 Qt 对象树内存管理
     */
    explicit LocationIndicatorManager(MapContainer* container);
    void initMap(QMapLibre::Map* map);

    struct LocationData {
        double latitude;
        double longitude;
        std::optional<double> heading; // 车头绝对朝向或 GPS 航向 (0-360)
        std::optional<double> speed;   // 速度 (m/s)，用于内部动态视口计算
    };

    /**
     * @brief 设置当前位置数据
     *
     * 更新位置指示器的 GPS 坐标。
     * - Free 模式：图标在地图上移到新坐标
     * - Fixed 模式：地图平移使该坐标对准屏幕固定点
     *
     * @param data
     *
     * @see setMode(), showLocation()
     */
    void setLocation(const LocationData& data);

    /**
     * @brief 获取当前位置数据
     * @return 当前位置数据
     * @see setLocation()
     */
    const LocationData& location() const;

    /**
     * @brief 设置位置指示器图标
     *
     * @param icon 图标图片，建议使用正方形 PNG（带透明通道）
     *
     * @code
     * manager->setLocationIcon(QImage(":/icons/location_arrow.png"));
     * @endcode
     */
    void setLocationIcon(const QImage& icon);

    /**
     * @brief 设置位置指示器模式
     *
     * - Free：图标渲染在地图坐标上，随地图移动（浏览模式）
     * - Fixed：图标固定在屏幕位置，地图跟随移动（导航模式）
     *
     * @param mode LocationMode::Free 或 LocationMode::Fixed
     *
     * @see LocationMode
     */
    void setMode(LocationMode mode);

    /**
     * @brief 获取当前显示模式
     * @return 当前 LocationMode
     * @see setMode()
     */
    LocationMode mode() const;

    /**
     * @brief 设置 Fixed 模式的导航朝向策略（车头朝上/正北朝上）
     */
    void setFixedHeadingMode(FixedHeadingMode mode);
    FixedHeadingMode fixedHeadingMode() const;

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
     * manager->setCenterOffset(200);
     * @endcode
     *
     * @see setMode()
     */
    void setCenterOffset(int bottomPixels);

    /**
     * @brief 获取当前中心偏移量
     * @return 偏移像素数
     * @see setCenterOffset()
     */
    int centerOffset() const;

    /**
     * @brief 设置目标缩放级别（Fixed 模式下生效）
     */
    void setZoom(double zoom);

    /**
     * @brief 设置目标俯仰角（Fixed 模式下生效）
     */
    void setPitch(double pitch);

    /**
     * @brief 获取目标缩放级别
     * @return 缩放级别，-1 表示未设置
     */
    double zoom() const { return m_targetZoom; }

    /**
     * @brief 获取目标俯仰角
     * @return 俯仰角，-1 表示未设置
     */
    double pitch() const { return m_targetPitch; }

    /** 地图跟随平滑度 (地图追GPS位置的lerp因子，默认0.15，导航时设0.05) */
    void setFollowSmoothFactor(double factor);
    double followSmoothFactor() const;
    static constexpr double DEFAULT_FOLLOW_SMOOTH_FACTOR = 0.15;

    /** 图标动画平滑度 (Free/FixedBrowsing下图标位置lerp因子，默认0.15) */
    void setIconSmoothFactor(double factor);
    double iconSmoothFactor() const;
    static constexpr double DEFAULT_ICON_SMOOTH_FACTOR = 0.15;

    /** 恢复超时时间 (ms)，拖动地图后自动恢复Fixed跟随的等待时间，默认3000 */
    void setFixedTouchResumeTimeout(int ms);
    int fixedTouchResumeTimeout() const;

    /** 暂停跟随 (MapContainer手势拦截时调用，幂等) */
    void pauseFollowing();

    /** Fixed 模式下更新图标 GeoJSON 到指定坐标（lerp 后的地图中心），保持图标固定在屏幕位置 */
    void updateSourceToCoordinate(double lat, double lon);

    bool isReady() const { return m_ready; }
    bool isLayerSetup() const { return m_layerSetup; }

signals:
    void locationChanged(const LocationData& data);
    void followingPausedChanged(bool paused);


private:
    /**
     * @brief 位置指示器内部状态
     */
    enum class State {
        Hidden,         ///< 位置指示器隐藏
        FreeVisible,    ///< Free 模式可见（图标在地图坐标上）
        FixedFollowing, ///< Fixed 模式 — 地图跟随 GPS
        FixedBrowsing   ///< Fixed 模式 — 用户正在拖拽地图
    };

    /**
     * @brief 获取当前内部状态
     * @return 当前 State
     */
    State state() const;

    void updateInteractionEnabled();
    void ensureLayerSetup();
    void rebuildSource();
    QByteArray buildGeoJson() const;
    void applyFixedMode();
    void applyFreeMode();
    void updateOverlayRotation();
    void repositionOverlay();
    void onIconAnimStep();
    void onFollowStep();
    void safeSetCoordinate(double lat, double lon);
    void safeSetBearing(double bearing);
    int effectiveCenterOffset() const;
    void setSelfAnimating(bool v) { m_selfAnimating = v; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

    QMapLibre::Map* m_map;
    bool m_ready = false;
    bool m_layerSetup = false;
    LocationData m_currentLocation{0.0, 0.0};
    LocationMode m_mode = LocationMode::Free;
    bool m_visible = false;
    QImage m_icon;
    double m_rotation = 0.0;  ///< 当前旋转角度（度）
    int m_centerOffset = 0;
    double m_targetZoom = -1.0;
    double m_targetPitch = -1.0;
    State m_state = State::Hidden;
    FixedHeadingMode m_fixedHeadingMode = FixedHeadingMode::HeadingUp;
    bool m_selfAnimating = false; ///< 区分自己触发的 map 变化 vs 用户拖拽

    // Follow timer (16ms) for smooth map tracking
    QTimer* m_followTimer = nullptr;
    // Resume timer (single-shot) for restoring Fixed mode after user drag
    QTimer* m_resumeTimer = nullptr;
    double m_followTargetLat = 0.0;
    double m_followTargetLon = 0.0;
    double m_targetBearing = -1.0;   // -1 = unset
    double m_followSmoothFactor = DEFAULT_FOLLOW_SMOOTH_FACTOR;
    double m_iconSmoothFactor = DEFAULT_ICON_SMOOTH_FACTOR;
    int m_fixedTouchResumeTimeout = 3000;
    bool m_followingPaused = false;

    // Icon position animation (FreeVisible / FixedBrowsing)
    double m_displayLat = 0.0;
    double m_displayLon = 0.0;
    QTimer* m_iconAnimTimer = nullptr;

    // Overlay widget for Fixed mode (screen-pinned icon)
    QLabel *m_overlay = nullptr;
    QWidget *m_parentWidget = nullptr;
    MapContainer* m_mapContainer = nullptr;
};

#endif
