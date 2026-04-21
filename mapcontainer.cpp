/*
 * mapcontainer.cpp - MapContainer 实现文件
 *
 * 本文件包含 MapContainer 的完整实现，主要功能：
 * 1. 地图初始化和配置（Settings、GLWidget、布局）
 * 2. 地图状态跟踪（通过 mapChanged 信号同步 zoom、bearing、pitch、center）
 * 3. 触摸手势识别（单指拖拽、双指缩放/旋转/平移）
 * 4. 触摸与鼠标事件的冲突预防
 *
 * 手势识别算法说明：
 * - 单指拖拽：计算连续两帧触摸点位置差，调用 moveBy() 平移地图
 * - 双指缩放：计算两指间距离比值（currDist / prevDist），调用 scaleBy() 缩放
 * - 双指旋转：计算两指连线角度差，调用 rotateBy() 旋转地图
 * - 双指平移：计算两指中心点移动差值，调用 moveBy() 平移地图
 *
 * 状态机：m_touchActive 标记触摸手势生命周期
 *   false -> TouchBegin -> true -> TouchUpdate ... -> TouchEnd/TouchCancel -> false
 */

#include "mapcontainer.h"
#include <QMapLibreWidgets/GLWidget>
#include <QMapLibre/Map>
#include <QMapLibre/Settings>
#include <QMapLibre/Types>
#include <QVBoxLayout>
#include <QTouchEvent>
#include <QLineF>
#include <cmath>

MapContainer::MapContainer(const MapConfig &config, QWidget *parent)
    : QWidget(parent)
    , m_glWidget(nullptr)
{
    // ============================================================
    // 步骤1: 配置 QMapLibre::Settings
    // 设置默认坐标（latitude, longitude 顺序）和默认缩放级别
    // 如果提供了 styleUrl，则配置地图样式（名称为 "HXGIS Day"）
    // ============================================================
    // Configure settings
    QMapLibre::Settings settings;
    // Coordinate order is (latitude, longitude)
    // 坐标顺序为 (纬度, 经度)
    settings.setDefaultCoordinate(config.defaultCoordinate);
    settings.setDefaultZoom(config.defaultZoom);
    if (!config.styleUrl.isEmpty()) {
        settings.setStyles(QMapLibre::Styles{
            QMapLibre::Style(config.styleUrl, QStringLiteral("HXGIS Day"))
        });
    }

    // ============================================================
    // 步骤2: 创建 GLWidget 实例
    // GLWidget 是 QMapLibre 的 OpenGL 渲染组件，负责地图的底层渲染
    // ============================================================
    m_glWidget = new QMapLibre::GLWidget(settings);

    // ============================================================
    // 步骤3: 设置布局
    // 使用 QVBoxLayout，边距设为0使地图填满整个容器
    // ============================================================
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_glWidget);

    // ============================================================
    // 步骤4: 连接 mapChanged 信号进行状态跟踪
    // QMapLibre::Map 使用单一的 mapChanged(MapChange) 信号而非每个属性的独立信号
    // Region change 事件涵盖所有相机移动（缩放、旋转、倾斜、平移）
    //
    // 状态跟踪逻辑：
    // 1. 仅处理 MapChangeRegionDidChange 和 MapChangeRegionDidChangeAnimated 事件
    // 2. 将当前地图状态与上一次保存的状态对比
    // 3. 如果发生变化，更新缓存值并发射对应的 Qt 信号通知外部
    // 4. 这种设计避免了不必要的信号发射，优化性能
    // ============================================================
    QMapLibre::Map *m = m_glWidget->map();
    connect(m, &QMapLibre::Map::mapChanged, this, [this, m](QMapLibre::Map::MapChange change) {
        // 只处理区域变化事件（包括动画和非动画）
        if (change != QMapLibre::Map::MapChangeRegionDidChange &&
            change != QMapLibre::Map::MapChangeRegionDidChangeAnimated)
            return;

        // 检查并同步 zoom（缩放级别）
        if (m->zoom() != m_lastZoom) {
            m_lastZoom = m->zoom();
            emit zoomChanged(m_lastZoom);
        }
        // 检查并同步 bearing（方位角/旋转角度）
        if (m->bearing() != m_lastBearing) {
            m_lastBearing = m->bearing();
            emit bearingChanged(m_lastBearing);
        }
        // 检查并同步 pitch（倾斜角度）
        if (m->pitch() != m_lastPitch) {
            m_lastPitch = m->pitch();
            emit tiltChanged(m_lastPitch);
        }
        // 检查并同步 center（中心坐标）
        auto coord = m->coordinate();
        if (coord.first != m_lastLat || coord.second != m_lastLon) {
            m_lastLat = coord.first;
            m_lastLon = coord.second;
            emit centerChanged(coord.first, coord.second);
        }
    });

    // ============================================================
    // 步骤5: 初始化状态缓存变量
    // 这些变量用于 mapChanged 信号处理中的状态对比
    // 初始值必须与 Settings 中配置的值保持一致
    // ============================================================
    m_lastZoom = settings.defaultZoom();
    m_lastBearing = 0.0;
    m_lastPitch = 0.0;
    m_lastLat = settings.defaultCoordinate().first;
    m_lastLon = settings.defaultCoordinate().second;

    // ============================================================
    // 步骤6: 启用触摸事件接收
    // Qt 默认不接收触摸事件，需要显式设置 WA_AcceptTouchEvents 属性
    // 这是触摸手势识别的前提条件
    // ============================================================
    setAttribute(Qt::WA_AcceptTouchEvents);
}

void MapContainer::setStyle(const QString &styleUrl) {
    m_glWidget->map()->setStyleUrl(styleUrl);
}

void MapContainer::setCenter(double lat, double lon) {
    m_glWidget->map()->setCoordinate(QMapLibre::Coordinate(lat, lon));
}

void MapContainer::setZoom(double zoom) {
    m_glWidget->map()->setZoom(zoom);
}

void MapContainer::setBearing(double bearing) {
    m_glWidget->map()->setBearing(bearing);
}

void MapContainer::setPitch(double pitch) {
    m_glWidget->map()->setPitch(pitch);
}

QMapLibre::Map *MapContainer::map() const {
    return m_glWidget->map();
}

bool MapContainer::event(QEvent *event) {
    switch (event->type()) {
    // ============================================================
    // TouchBegin: 手势开始
    // 状态机转换: m_touchActive = false -> true
    //
    // 处理逻辑:
    // 1. 将 m_touchActive 设为 true，标记触摸手势正在进行
    // 2. 记录当前触摸点数量和坐标，作为后续手势计算的基准
    // 3. 调用 setGestureInProgress(true) 通知地图引擎手势开始
    //    这会禁用地图的惯性动画，确保手势响应直接可控
    // 4. accept() 事件，阻止事件继续向上层传播
    // ============================================================
    case QEvent::TouchBegin: {
        auto *touchEvent = static_cast<QTouchEvent *>(event);
        m_touchActive = true;
        m_touchPointCount = touchEvent->points().count();
        m_lastTouchPoints = touchEvent->points();
        map()->setGestureInProgress(true);
        event->accept();
        return true;
    }
    // ============================================================
    // TouchUpdate: 手势进行中（核心手势识别逻辑）
    // 状态机: m_touchActive 保持 true
    //
    // 手势识别算法:
    // 1. 单指拖拽（m_touchPointCount == 1）:
    //    - 计算当前帧与上一帧触摸点的位置差 delta
    //    - delta = points.first().position() - m_lastTouchPoints.first().position()
    //    - 调用 moveBy(delta) 平移地图
    //
    // 2. 双指手势（m_touchPointCount == 2）:
    //    需要上一帧也有两个点才能计算差值
    //
    //    a) 双指缩放（Pinch Zoom）:
    //       - 计算当前两指间距离: currDist = QLineF(p1, p2).length()
    //       - 计算上一帧两指间距离: prevDist = QLineF(prevP1, prevP2).length()
    //       - 缩放因子: scaleFactor = currDist / prevDist
    //         > 1.0 表示放大（手指分开），< 1.0 表示缩小（手指靠拢）
    //       - 缩放中心: center = (p1 + p2) / 2.0（两指中点）
    //       - 调用 scaleBy(scaleFactor, center) 执行缩放
    //
    //    b) 双指旋转（Rotation）:
    //       - 计算当前两指连线角度: currLine.angle()
    //       - 计算上一帧两指连线角度: prevLine.angle()
    //       - 角度差: angleDelta = currLine.angle() - prevLine.angle()
    //       - 如果角度差绝对值 > 0.1 度，调用 rotateBy(p1, p2) 旋转地图
    //       - 0.1 度的阈值用于过滤微小抖动，避免误触发旋转
    //
    //    c) 双指平移（Two-finger Pan）:
    //       - 计算当前两指中心: (p1 + p2) / 2.0
    //       - 计算上一帧两指中心: (prevP1 + prevP2) / 2.0
    //       - 中心点移动差: centerDelta = 当前中心 - 上一帧中心
    //       - 如果 centerDelta.manhattanLength() > 0，调用 moveBy(centerDelta)
    //       - manhattanLength() 使用 L1 范数，计算效率高且避免开方
    //
    // 3. 更新 m_lastTouchPoints 为当前帧数据，供下一帧计算使用
    // ============================================================
    case QEvent::TouchUpdate: {
        auto *touchEvent = static_cast<QTouchEvent *>(event);
        const auto &points = touchEvent->points();
        m_touchPointCount = points.count();

        // 单指拖拽: 计算位置差并平移地图
        if (m_touchPointCount == 1 && m_lastTouchPoints.count() >= 1) {
            QPointF delta = points.first().position() - m_lastTouchPoints.first().position();
            map()->moveBy(delta);
        // 双指手势: 需要当前帧和上一帧都有两个触摸点
        } else if (m_touchPointCount == 2 && m_lastTouchPoints.count() >= 2) {
            const QPointF &p1 = points.at(0).position();
            const QPointF &p2 = points.at(1).position();
            const QPointF &prevP1 = m_lastTouchPoints.at(0).position();
            const QPointF &prevP2 = m_lastTouchPoints.at(1).position();

            // 双指缩放: 距离比值计算
            qreal currDist = QLineF(p1, p2).length();
            qreal prevDist = QLineF(prevP1, prevP2).length();
            if (prevDist > 0 && currDist > 0) {
                qreal scaleFactor = currDist / prevDist;
                QPointF center = (p1 + p2) / 2.0;
                map()->scaleBy(scaleFactor, center);
            }

            // 双指旋转: 角度差计算
            QLineF currLine(p1, p2);
            QLineF prevLine(prevP1, prevP2);
            qreal angleDelta = currLine.angle() - prevLine.angle();
            if (std::abs(angleDelta) > 0.1) {
                map()->rotateBy(p1, p2);
            }

            // 双指平移: 中心点移动
            QPointF centerDelta = ((p1 + p2) / 2.0) - ((prevP1 + prevP2) / 2.0);
            if (centerDelta.manhattanLength() > 0) {
                map()->moveBy(centerDelta);
            }
        }

        // 保存当前帧触摸点，供下一帧使用
        m_lastTouchPoints = points;
        event->accept();
        return true;
    }
    // ============================================================
    // TouchEnd / TouchCancel: 手势结束/取消
    // 状态机转换: m_touchActive = true -> false
    //
    // 清理逻辑:
    // 1. m_touchActive = false: 标记触摸手势结束
    // 2. m_touchPointCount = 0: 重置触摸点计数
    // 3. m_lastTouchPoints.clear(): 清空触摸点缓存
    // 4. setGestureInProgress(false): 通知地图引擎手势结束
    //    这会重新启用地图的惯性动画和自动行为
    // 5. accept() 事件，完成事件处理
    // ============================================================
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        m_touchActive = false;
        m_touchPointCount = 0;
        m_lastTouchPoints.clear();
        map()->setGestureInProgress(false);
        event->accept();
        return true;
    }
    default:
        return QWidget::event(event);
    }
}

// ============================================================
// 鼠标事件处理: 触摸/鼠标冲突预防
//
// 问题背景:
// 在支持触摸的设备上，触摸手势会同时产生触摸事件和模拟的鼠标事件。
// 如果不做处理，会导致手势被重复处理（一次触摸 + 一次鼠标），
// 造成地图移动/缩放异常。
//
// 解决方案:
// 使用 m_touchActive 状态机进行冲突预防:
// - 当 m_touchActive == true 时（触摸手势进行中），忽略所有鼠标事件
// - 当 m_touchActive == false 时，正常处理鼠标事件
//
// 实现方式:
// 1. 检查 m_touchActive 状态
// 2. 如果为 true，调用 event->ignore() 忽略事件，阻止事件继续传播
// 3. 如果为 false，调用父类的默认实现（QWidget::mouseXxxEvent）
//
// 覆盖的事件:
// - mousePressEvent: 鼠标按下
// - mouseMoveEvent: 鼠标移动
// - mouseReleaseEvent: 鼠标释放
// - wheelEvent: 滚轮事件（触摸板双指滚动也会触发）
// ============================================================

void MapContainer::mousePressEvent(QMouseEvent *event) {
    // 触摸手势进行中时忽略鼠标按下事件，防止冲突
    if (m_touchActive) {
        event->ignore();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MapContainer::mouseMoveEvent(QMouseEvent *event) {
    // 触摸手势进行中时忽略鼠标移动事件，防止冲突
    if (m_touchActive) {
        event->ignore();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void MapContainer::mouseReleaseEvent(QMouseEvent *event) {
    // 触摸手势进行中时忽略鼠标释放事件，防止冲突
    if (m_touchActive) {
        event->ignore();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void MapContainer::wheelEvent(QWheelEvent *event) {
    // 触摸手势进行中时忽略滚轮事件，防止冲突
    // 注意: 触摸板双指滚动在某些平台上也会触发 wheelEvent
    if (m_touchActive) {
        event->ignore();
        return;
    }
    QWidget::wheelEvent(event);
}
