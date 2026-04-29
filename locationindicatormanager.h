/**
 * @file locationindicatormanager.h
 * @brief 位置指示器管理器
 *
 * 管理当前位置图标的渲染，支持两种互斥模式：
 *
 * - Free（自由定位）：图标渲染在地图上的实际 GPS 坐标位置，
 *   使用 Symbol Layer + GeoJSON 实现。随地图移动/缩放而变化。
 *   适合普通浏览场景。
 *
 * - Fixed（固定中心）：图标固定在屏幕上（如底部 1/3 处），
 *   地图平移跟随位置。使用 Qt overlay widget + setMargins 实现。
 *   适合导航场景。
 *
 * @code
 * // 1. 设置位置图标
 * mapContainer->setLocationIcon(QImage(":/icons/location.png"));
 *
 * // 2. Free 模式（默认）
 * mapContainer->setLocation(39.9042, 116.4074);
 * mapContainer->showLocation();
 *
 * // 3. 切换到导航模式（Fixed）
 * mapContainer->setCenterOffset(200);    // 中心下移 200px
 * mapContainer->setLocationMode(LocationIndicatorManager::Fixed);
 * mapContainer->setLocation(39.9050, 116.4080);  // 地图自动跟随
 *
 * // 4. 切回浏览模式
 * mapContainer->setLocationMode(LocationIndicatorManager::Free);
 *
 * // 5. 隐藏
 * mapContainer->hideLocation();
 * @endcode
 */

#ifndef LOCATIONINDICATORMANAGER_H
#define LOCATIONINDICATORMANAGER_H

#include <QObject>
#include <QImage>
#include <QWidget>

namespace QMapLibre { class Map; }

/**
 * @brief 位置指示器管理器
 *
 * 负责管理当前位置图标在地图上的渲染方式和交互行为。
 * 通过 LocationMode 枚举切换 Free/Fixed 两种显示模式。
 *
 * @see LocationMode
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
     * @brief 构造位置指示器管理器
     *
     * @param map    QMapLibre::Map 实例指针，用于操作地图图层和源
     * @param parent 父对象，用于 Qt 对象树内存管理
     */
    explicit LocationIndicatorManager(QMapLibre::Map* map, QObject* parent = nullptr);

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
     * @see setMode(), showLocation()
     */
    void setLocation(double lat, double lon);

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
     * @brief 通知地图就绪状态
     *
     * 地图加载完成后调用，允许管理器开始设置图层和源。
     *
     * @param ready true 表示地图已就绪
     */
    void setMapReady(bool ready);

    /**
     * @brief 设置 Fixed 模式使用的覆盖 Widget
     *
     * 将外部 QLabel 作为 Fixed 模式下的位置图标覆盖层。
     * 管理器会根据 setCenterOffset() 自动调整其位置。
     *
     * @param overlay 覆盖 Widget 指针（通常为 QLabel）
     */
    void setOverlayWidget(QWidget* overlay);

    /**
     * @brief 重新定位覆盖 Widget
     *
     * 根据当前 centerOffset 和父窗口大小重新计算覆盖层位置。
     * 通常在窗口 resize 事件中调用。
     */
    void repositionOverlay();

private:
    void ensureLayerSetup();
    void rebuildSource();
    QByteArray buildGeoJson() const;
    void applyFixedMode();
    void applyFreeMode();

    QMapLibre::Map* m_map;
    QWidget* m_overlay = nullptr;
    bool m_ready = false;
    bool m_layerSetup = false;
    double m_lat = 0.0;
    double m_lon = 0.0;
    LocationMode m_mode = LocationMode::Free;
    bool m_visible = false;
    QImage m_icon;
    int m_centerOffset = 0;
};

#endif
