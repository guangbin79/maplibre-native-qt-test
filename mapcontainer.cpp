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
#include <QVBoxLayout>
#include <QTouchEvent>
#include <QMapLibreWidgets/GLWidget>
#include <QMapLibre/Map>
#include <QDateTime>
#include <QTimer>
#include <cmath>
#include <QResizeEvent>

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
    // 限制 tile 缓存数据库大小为 50MB
    settings.setCacheDatabaseMaximumSize(50 * 1024 * 1024);
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
    // 步骤4: 延迟连接 mapChanged 信号
    // QMapLibre::GLWidget 构造时 map() 可能返回 nullptr，
    // 需要等待 GL 上下文初始化完成后再连接信号
    // ============================================================
    QTimer::singleShot(0, this, &MapContainer::connectMapSignals);

    // ============================================================
    // 步骤7: 启用触摸事件接收
    // Qt 默认不接收触摸事件，需要显式设置 WA_AcceptTouchEvents 属性
    // 这是触摸手势识别的前提条件
    // ============================================================
    setAttribute(Qt::WA_AcceptTouchEvents);

    // 相机动画定时器
    m_cameraAnimTimer = new QTimer(this);
    m_cameraAnimTimer->setInterval(16);  // ~60fps
    m_cameraAnimTimer->setSingleShot(false);
    connect(m_cameraAnimTimer, &QTimer::timeout, this, &MapContainer::onCameraAnimStep);

    // GPS跟随平滑定时器
    m_followTimer = new QTimer(this);
    m_followTimer->setInterval(16);
    m_followTimer->setSingleShot(false);
    connect(m_followTimer, &QTimer::timeout, this, &MapContainer::onFollowStep);

    // Fixed模式触屏恢复定时器
    m_fixedResumeTimer = new QTimer(this);
    m_fixedResumeTimer->setSingleShot(true);
    connect(m_fixedResumeTimer, &QTimer::timeout, this, [this]() {
        if (!m_fixedPausedByTouch) return;
        m_fixedPausedByTouch = false;
        m_locationIndicatorManager->setFollowingPaused(false);
        m_locationIndicatorManager->restoreFixedDisplay();
        auto loc = m_locationIndicatorManager->location();
        animateTo(loc.first, loc.second, map()->zoom(), map()->bearing(), map()->pitch(), 500);
    });
}

void MapContainer::setStyle(const QString &styleUrl) {
    m_glWidget->map()->setStyleUrl(styleUrl);
}

void MapContainer::setCenter(double lat, double lon) {
    m_glWidget->map()->setCoordinate(QMapLibre::Coordinate(lat, lon));
}

void MapContainer::setZoom(double zoom) {
    m_glWidget->map()->setZoom(qBound(0.0, zoom, MAX_ZOOM));
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
        // Fixed mode touch pan: pause following to allow free map browsing
        if (m_fixedTouchPanEnabled
            && m_locationIndicatorManager->mode() == LocationIndicatorManager::LocationMode::Fixed
            && m_locationIndicatorManager->isLocationVisible()) {
            m_fixedResumeTimer->stop();
            m_fixedPausedByTouch = true;
            m_locationIndicatorManager->setFollowingPaused(true);
            m_followTimer->stop();
            m_locationIndicatorManager->showLocationOnMap();
        }
        stopCameraAnimation();
        auto *touchEvent = static_cast<QTouchEvent *>(event);
        const auto &points = touchEvent->points();
        m_touchActive = true;
        m_touchPointCount = points.count();
        m_lastTouchPoints = points;

        // 双击检测：单指按下时检查是否与上次触摸结束构成双击
        if (m_touchPointCount == 1) {
            qint64 pressTime = static_cast<qint64>(points.first().pressTimestamp());
            QPointF pos = points.first().position();
            qint64 timeDelta = pressTime - m_lastTouchEndTime;
            qreal dist = QLineF(pos, m_lastTouchEndPos).length();
            if (timeDelta > 0 && timeDelta < DOUBLE_TAP_INTERVAL_MS && dist < DOUBLE_TAP_DISTANCE_PX) {
                m_doubleTapAnimCenter = pos;
                double targetZoom = qMin(map()->zoom() + 1.0, MAX_ZOOM);
                QMapLibre::Coordinate center = map()->coordinate();
                animateTo(center.first, center.second, targetZoom, map()->bearing(), map()->pitch(), 160);
                m_touchActive = false;
                event->accept();
                return true;
            }
        }

        // 双指按下时初始化手势识别状态
        if (m_touchPointCount == 2) {
            m_gestureMode = GestureMode::None;
            m_accumulatedRotation = 0.0;
            m_rotationSkipCounter = 0;
            m_panSkipCounter = 0;
            const auto &p1 = points.at(0).position();
            const auto &p2 = points.at(1).position();
            m_initialPinchDist = QLineF(p1, p2).length();
            m_initialPinchAngle = QLineF(p1, p2).angle();
            m_initialPinchCenter = (p1 + p2) / 2.0;
            // 记录双指点击检测的起始状态（使用第一指的 pressTimestamp）
            m_twoFingerTapStartTime = static_cast<qint64>(points.at(0).pressTimestamp());
            m_twoFingerTapStartPos1 = p1;
            m_twoFingerTapStartPos2 = p2;
            m_twoFingerTapInitialDist = m_initialPinchDist;
        }

        map()->setGestureInProgress(true);
        emit touchBegin();
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
        qint64 touchUpdateStart = QDateTime::currentMSecsSinceEpoch();
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

            // 如果初始 pinch 参数未初始化（TouchBegin 只有 1 个点），在这里补初始化
            if (m_initialPinchDist <= 0.0) {
                m_initialPinchDist = QLineF(p1, p2).length();
                m_initialPinchAngle = QLineF(p1, p2).angle();
                m_initialPinchCenter = (p1 + p2) / 2.0;
            }

            // 手势模式识别：前 3 帧内判断用户主导意图并锁定
            if (m_gestureMode == GestureMode::None && m_initialPinchDist > 10.0) {
                qreal currDist = QLineF(p1, p2).length();
                qreal currAngle = QLineF(p1, p2).angle();
                qreal distChange = std::abs(currDist - m_initialPinchDist) / m_initialPinchDist;
                qreal angleChange = currAngle - m_initialPinchAngle;
                while (angleChange > 180.0) angleChange -= 360.0;
                while (angleChange < -180.0) angleChange += 360.0;
                angleChange = std::abs(angleChange);

                QPointF currCenter = (p1 + p2) / 2.0;
                QPointF centerDelta = currCenter - m_initialPinchCenter;
                qreal absDx = std::abs(centerDelta.x());
                qreal absDy = std::abs(centerDelta.y());

                if (absDy > absDx * 0.5 && distChange < 0.20) {
                    m_gestureMode = GestureMode::Pitch;
                } else if (distChange > 0.04 && angleChange < 5.0) {
                    m_gestureMode = GestureMode::Scale;
                } else if (angleChange > 5.0 && distChange < 0.04) {
                    m_gestureMode = GestureMode::Rotate;
                } else if (distChange > 0.04 && angleChange > 5.0) {
                    m_gestureMode = GestureMode::Both;
                }
            }

            if (m_gestureMode == GestureMode::Pitch) {
                QPointF currCenter = (p1 + p2) / 2.0;
                QPointF prevCenter = (prevP1 + prevP2) / 2.0;
                qreal dy = currCenter.y() - prevCenter.y();
                if (std::abs(dy) > 0.5) {
                    double pitchDelta = -dy * PITCH_SENSITIVITY;
                    double newPitch = qBound(MIN_PITCH, map()->pitch() + pitchDelta, MAX_PITCH);
                    map()->setPitch(newPitch);
                    if (newPitch != m_lastPitch) {
                        m_lastPitch = newPitch;
                        emit tiltChanged(m_lastPitch);
                    }
                }
            } else if (m_gestureMode == GestureMode::Rotate) {
                // ── 纯旋转模式 ──
                QLineF currLine(p1, p2);
                QLineF prevLine(prevP1, prevP2);
                qreal angleDelta = currLine.angle() - prevLine.angle();
                while (angleDelta > 180.0) angleDelta -= 360.0;
                while (angleDelta < -180.0) angleDelta += 360.0;
                if (std::abs(angleDelta) > 0.5) {
                    m_accumulatedRotation += angleDelta;
                    ++m_rotationSkipCounter;
                    if (m_rotationSkipCounter % 3 == 0 || std::abs(m_accumulatedRotation) > 2.0) {
                        qint64 t1 = QDateTime::currentMSecsSinceEpoch();
                        map()->rotateBy(prevP1, p1);
                        qint64 t2 = QDateTime::currentMSecsSinceEpoch();
                        qDebug() << "[PERF] rotateBy() ms=" << (t2 - t1) << "zoom=" << map()->zoom() << "bearing=" << map()->bearing();
                        m_accumulatedRotation = 0.0;
                    }
                }
            } else {
                // ── 缩放 + 平移模式（Scale / Both / None）──
                if (m_gestureMode == GestureMode::None ||
                    m_gestureMode == GestureMode::Scale ||
                    m_gestureMode == GestureMode::Both) {
                    qreal currDist = QLineF(p1, p2).length();
                    qreal prevDist = QLineF(prevP1, prevP2).length();
                    if (prevDist > 10.0 && currDist > 10.0) {
                        qreal scaleFactor = currDist / prevDist;
                        scaleFactor = qBound(0.85, scaleFactor, 1.2);
                        QPointF center = (p1 + p2) / 2.0;
                        map()->scaleBy(scaleFactor, center);
                        double newZoom = map()->zoom();
                        if (newZoom > MAX_ZOOM) {
                            map()->setZoom(MAX_ZOOM);
                            newZoom = MAX_ZOOM;
                        }
                        if (newZoom != m_lastZoom) {
                            m_lastZoom = newZoom;
                            emit zoomChanged(m_lastZoom);
                        }
                    }
                }

                // ── 平移（所有非纯旋转模式都允许）──
                QPointF centerDelta = ((p1 + p2) / 2.0) - ((prevP1 + prevP2) / 2.0);
                if (centerDelta.manhattanLength() > 0.5) {
                    ++m_panSkipCounter;
                    if (m_panSkipCounter % 2 == 0) {
                        map()->moveBy(centerDelta);
                    }
                }

                // ── 旋转（仅在 Both / None 模式下执行）──
                if (m_gestureMode == GestureMode::None || m_gestureMode == GestureMode::Both) {
                    QLineF currLine(p1, p2);
                    QLineF prevLine(prevP1, prevP2);
                    qreal angleDelta = currLine.angle() - prevLine.angle();
                    while (angleDelta > 180.0) angleDelta -= 360.0;
                    while (angleDelta < -180.0) angleDelta += 360.0;
                    if (std::abs(angleDelta) > 0.5) {
                        m_accumulatedRotation += angleDelta;
                        ++m_rotationSkipCounter;
                        if (m_rotationSkipCounter % 3 == 0 || std::abs(m_accumulatedRotation) > 2.0) {
                            qint64 t1 = QDateTime::currentMSecsSinceEpoch();
                            map()->rotateBy(prevP1, p1);
                            qint64 t2 = QDateTime::currentMSecsSinceEpoch();
                            qDebug() << "[PERF] rotateBy() ms=" << (t2 - t1) << "zoom=" << map()->zoom() << "bearing=" << map()->bearing();
                            m_accumulatedRotation = 0.0;
                        }
                    }
                }
            }
        }

        // 保存当前帧触摸点，供下一帧使用
        m_lastTouchPoints = points;
        event->accept();
        qint64 touchUpdateEnd = QDateTime::currentMSecsSinceEpoch();
        if (m_touchPointCount == 2) {
            qDebug() << "[PERF] TouchUpdate total ms=" << (touchUpdateEnd - touchUpdateStart)
                     << "gestureMode=" << static_cast<int>(m_gestureMode);
        }
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
        // 记录双击检测信息（仅在单指触摸结束时）
        if (m_touchPointCount == 1 && !m_lastTouchPoints.isEmpty()) {
            m_lastTouchEndTime = static_cast<qint64>(m_lastTouchPoints.first().timestamp());
            m_lastTouchEndPos = m_lastTouchPoints.first().position();
        }

        // 双指点击缩小检测：两指同时轻触后快速抬起，无明显移动
        if (m_touchPointCount == 2
            && m_gestureMode == GestureMode::None
            && m_twoFingerTapStartTime > 0
            && !m_lastTouchPoints.isEmpty()
            && m_lastTouchPoints.count() >= 2) {
            qint64 endTime = static_cast<qint64>(m_lastTouchPoints.first().timestamp());
            qint64 duration = endTime - m_twoFingerTapStartTime;
            if (duration > 0 && duration < TWO_FINGER_TAP_DURATION_MS) {
                // 检查手指漂移距离
                QPointF endPos1 = m_lastTouchPoints.at(0).position();
                QPointF endPos2 = m_lastTouchPoints.at(1).position();
                qreal drift1 = QLineF(m_twoFingerTapStartPos1, endPos1).length();
                qreal drift2 = QLineF(m_twoFingerTapStartPos2, endPos2).length();
                // 检查距离变化比例
                qreal endDist = QLineF(endPos1, endPos2).length();
                qreal distRatio = (m_twoFingerTapInitialDist > 10.0)
                                  ? std::abs(endDist - m_twoFingerTapInitialDist) / m_twoFingerTapInitialDist
                                  : 999.0;
                if (drift1 < TWO_FINGER_TAP_MAX_DRIFT_PX
                    && drift2 < TWO_FINGER_TAP_MAX_DRIFT_PX
                    && distRatio < TWO_FINGER_TAP_DIST_CHANGE_RATIO) {
                    // 检测到双指点击 → 缩小 1 级
                    double targetZoom = qMax(map()->zoom() - 1.0, 0.0);
                    if (targetZoom < map()->zoom()) {
                        QMapLibre::Coordinate center = map()->coordinate();
                        animateTo(center.first, center.second, targetZoom, map()->bearing(), map()->pitch(), 160);
                    }
                    // 防止手指抬起触发单指双击放大（交叉触发防护）
                    m_lastTouchEndTime = 0;
                    // 完整状态重置后提前返回
                    m_touchActive = false;
                    m_touchPointCount = 0;
                    m_lastTouchPoints.clear();
                    m_gestureMode = GestureMode::None;
                    m_accumulatedRotation = 0.0;
                    m_rotationSkipCounter = 0;
                    m_panSkipCounter = 0;
                    m_twoFingerTapStartTime = 0;
                    map()->setGestureInProgress(false);
                    if (m_fixedPausedByTouch) {
                        m_fixedResumeTimer->setInterval(m_fixedTouchResumeTimeout);
                        m_fixedResumeTimer->start();
                    }
                    emit touchEnd();
                    event->accept();
                    return true;
                }
            }
            m_twoFingerTapStartTime = 0; // 不是双指点击，重置
        }

        m_touchActive = false;
        m_touchPointCount = 0;
        m_lastTouchPoints.clear();
        m_gestureMode = GestureMode::None;
        m_accumulatedRotation = 0.0;
        m_rotationSkipCounter = 0;
        m_panSkipCounter = 0;
        map()->setGestureInProgress(false);
        // Restart resume timer if touch pan paused the following
        if (m_fixedPausedByTouch) {
            m_fixedResumeTimer->setInterval(m_fixedTouchResumeTimeout);
            m_fixedResumeTimer->start();
        }
        emit touchEnd();
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

void MapContainer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_locationIndicatorManager)
        m_locationIndicatorManager->repositionOverlay();
}

// ===== 标注管理委托方法 =====

void MapContainer::setAnnotations(const QVector<MapAnnotation>& annotations,
                                   const QMap<QString, QImage>& icons) {
    m_annotationManager->setAnnotations(annotations, icons);
}

void MapContainer::clearAnnotations() {
    m_annotationManager->clearAnnotations();
}

void MapContainer::addAnnotation(const MapAnnotation& annotation,
                                  const QImage& icon) {
    m_annotationManager->addAnnotation(annotation, icon);
}

void MapContainer::addAnnotations(const QVector<MapAnnotation>& annotations,
                                   const QMap<QString, QImage>& icons) {
    m_annotationManager->addAnnotations(annotations, icons);
}

void MapContainer::removeAnnotation(const QString& id) {
    m_annotationManager->removeAnnotation(id);
}

void MapContainer::removeAnnotations(const QStringList& ids) {
    m_annotationManager->removeAnnotations(ids);
}

void MapContainer::setVisibleIds(const QStringList& ids) {
    m_annotationManager->setVisibleIds(ids);
}

void MapContainer::showAllAnnotations() {
    m_annotationManager->showAllAnnotations();
}

void MapContainer::hideAllAnnotations() {
    m_annotationManager->hideAllAnnotations();
}

QStringList MapContainer::allIds() const {
    return m_annotationManager->allIds();
}

QStringList MapContainer::visibleIds() const {
    return m_annotationManager->visibleIds();
}

// ===== 线路管理委托方法 =====

void MapContainer::setRoutes(const QVector<MapRouteSegment>& segments) {
    m_routeManager->setSegments(segments);
}

void MapContainer::clearRoutes() {
    m_routeManager->clearSegments();
}

void MapContainer::addRouteSegment(const MapRouteSegment& segment) {
    m_routeManager->addRouteSegment(segment);
}

void MapContainer::addRouteSegments(const QVector<MapRouteSegment>& segments) {
    m_routeManager->addRouteSegments(segments);
}

void MapContainer::removeRouteSegment(const QString& id) {
    m_routeManager->removeRouteSegment(id);
}

void MapContainer::removeRouteSegments(const QStringList& ids) {
    m_routeManager->removeRouteSegments(ids);
}

void MapContainer::setVisibleRouteIds(const QStringList& routeIds) {
    m_routeManager->setVisibleRouteIds(routeIds);
}

void MapContainer::showAllRoutes() {
    m_routeManager->showAllRoutes();
}

void MapContainer::hideAllRoutes() {
    m_routeManager->hideAllRoutes();
}

QStringList MapContainer::allRouteIds() const {
    return m_routeManager->allRouteIds();
}

QStringList MapContainer::visibleRouteIds() const {
    return m_routeManager->visibleRouteIds();
}

// ===== 位置指示器委托方法 =====

void MapContainer::setLocation(double lat, double lon) {
    m_followTargetLat = lat;
    m_followTargetLon = lon;
    m_locationIndicatorManager->setLocation(lat, lon);

    if (m_locationIndicatorManager->mode() == LocationIndicatorManager::LocationMode::Fixed
        && !m_locationIndicatorManager->isFollowingPaused()) {
        if (!m_followTimer->isActive())
            m_followTimer->start();
    }
}

void MapContainer::setLocationIcon(const QImage& icon) {
    m_locationIndicatorManager->setLocationIcon(icon);
}

void MapContainer::setLocationRotation(double degrees) {
    m_locationIndicatorManager->setLocationRotation(degrees);
}

double MapContainer::locationRotation() const {
    return m_locationIndicatorManager->locationRotation();
}

void MapContainer::setLocationMode(LocationIndicatorManager::LocationMode mode) {
    m_locationIndicatorManager->setMode(mode);
    if (mode == LocationIndicatorManager::LocationMode::Fixed) {
        if (!m_locationIndicatorManager->isFollowingPaused())
            m_followTimer->start();
    } else {
        m_followTimer->stop();
    }
}

LocationIndicatorManager::LocationMode MapContainer::locationMode() const {
    return m_locationIndicatorManager->mode();
}

void MapContainer::showLocation() {
    m_locationIndicatorManager->showLocation();
}

void MapContainer::hideLocation() {
    m_locationIndicatorManager->hideLocation();
}

bool MapContainer::isLocationVisible() const {
    return m_locationIndicatorManager->isLocationVisible();
}

void MapContainer::setCenterOffset(int bottomPixels) {
    m_locationIndicatorManager->setCenterOffset(bottomPixels);
}

void MapContainer::setFixedTouchPanEnabled(bool enabled) {
    m_fixedTouchPanEnabled = enabled;
}

bool MapContainer::isFixedTouchPanEnabled() const {
    return m_fixedTouchPanEnabled;
}

void MapContainer::setFixedTouchResumeTimeout(int ms) {
    m_fixedTouchResumeTimeout = ms;
}

int MapContainer::fixedTouchResumeTimeout() const {
    return m_fixedTouchResumeTimeout;
}

void MapContainer::setDefaultAnimationDuration(int ms) {
    m_defaultAnimDuration = ms;
}

void MapContainer::animateTo(double lat, double lon, double zoom, double bearing, double pitch, int durationMs) {
    int duration = (durationMs < 0) ? m_defaultAnimDuration : durationMs;

    double targetZoom = CameraMath::clampedZoom(zoom);
    double targetPitch = CameraMath::clampedPitch(pitch);

    QMapLibre::Coordinate center = map()->coordinate();
    double currentLat = center.first;
    double currentLon = center.second;
    double currentZoom = map()->zoom();
    double currentBearing = map()->bearing();
    double currentPitch = map()->pitch();

    if (qFuzzyCompare(lat, currentLat) && qFuzzyCompare(lon, currentLon) &&
        qFuzzyCompare(targetZoom, currentZoom) && qFuzzyCompare(bearing, currentBearing) &&
        qFuzzyCompare(targetPitch, currentPitch)) {
        emit animationFinished();
        return;
    }

    stopCameraAnimation();

    m_animStartLat = currentLat;
    m_animStartLon = currentLon;
    m_animStartZoom = currentZoom;
    m_animStartBearing = currentBearing;
    m_animStartPitch = currentPitch;
    m_animTargetLat = lat;
    m_animTargetLon = lon;
    m_animTargetZoom = targetZoom;
    m_animTargetBearing = bearing;
    m_animTargetPitch = targetPitch;

    m_cameraAnimTotalSteps = qMax(1, duration / 16);
    m_cameraAnimStep = 0;

    m_cameraAnimTimer->start();
}

void MapContainer::onCameraAnimStep() {
    m_cameraAnimStep++;
    double t = CameraMath::easeInOutQuad(static_cast<double>(m_cameraAnimStep) / m_cameraAnimTotalSteps);

    double lat = CameraMath::lerp(m_animStartLat, m_animTargetLat, t);
    double lon = CameraMath::lerp(m_animStartLon, m_animTargetLon, t);
    double zoom = CameraMath::clampedZoom(CameraMath::lerp(m_animStartZoom, m_animTargetZoom, t));
    double bearing = m_animStartBearing + CameraMath::bearingDelta(m_animStartBearing, m_animTargetBearing) * t;
    double pitch = CameraMath::clampedPitch(CameraMath::lerp(m_animStartPitch, m_animTargetPitch, t));

    map()->setCoordinate(QMapLibre::Coordinate(lat, lon));
    map()->setZoom(zoom);
    map()->setBearing(bearing);
    map()->setPitch(pitch);

    m_lastLat = lat;
    m_lastLon = lon;
    m_lastZoom = zoom;
    m_lastBearing = bearing;
    m_lastPitch = pitch;

    if (m_cameraAnimStep >= m_cameraAnimTotalSteps) {
        stopCameraAnimation();
        emit animationFinished();
    }
}

void MapContainer::stopCameraAnimation() {
    m_cameraAnimTimer->stop();
    m_cameraAnimStep = 0;
}

void MapContainer::connectMapSignals()
{
    QMapLibre::Map *m = m_glWidget->map();
    if (!m) {
        qDebug() << "MapContainer: map() not ready, retrying in 100ms...";
        QTimer::singleShot(100, this, &MapContainer::connectMapSignals);
        return;
    }

    connect(m, &QMapLibre::Map::mapChanged, this, [this, m](QMapLibre::Map::MapChange change) {
        if (change == QMapLibre::Map::MapChangeDidFinishLoadingMap) {
            m_annotationManager->setMapReady(true);
            m_routeManager->setMapReady(true);
            m_locationIndicatorManager->setMapReady(true);
            m_mapReady = true;
            emit mapReady();
            return;
        }

        if (change != QMapLibre::Map::MapChangeRegionWillChange &&
            change != QMapLibre::Map::MapChangeRegionIsChanging &&
            change != QMapLibre::Map::MapChangeRegionDidChange &&
            change != QMapLibre::Map::MapChangeRegionDidChangeAnimated)
            return;

        qint64 mapChangeStart = QDateTime::currentMSecsSinceEpoch();
        if (m->zoom() != m_lastZoom) {
            m_lastZoom = m->zoom();
            emit zoomChanged(m_lastZoom);
        }
        if (m->bearing() != m_lastBearing) {
            m_lastBearing = m->bearing();
            emit bearingChanged(m_lastBearing);
        }
        if (m->pitch() != m_lastPitch) {
            m_lastPitch = m->pitch();
            emit tiltChanged(m_lastPitch);
        }
        auto coord = m->coordinate();
        if (coord.first != m_lastLat || coord.second != m_lastLon) {
            m_lastLat = coord.first;
            m_lastLon = coord.second;
            emit centerChanged(coord.first, coord.second);
        }
        qint64 mapChangeEnd = QDateTime::currentMSecsSinceEpoch();
        if ((mapChangeEnd - mapChangeStart) > 5) {
            qDebug() << "[PERF] mapChanged handler ms=" << (mapChangeEnd - mapChangeStart)
                     << "change=" << change;
        }
    });

    m_lastZoom = m->zoom();
    m_lastBearing = m->bearing();
    m_lastPitch = m->pitch();
    auto coord = m->coordinate();
    m_lastLat = coord.first;
    m_lastLon = coord.second;

    m_annotationManager = new AnnotationManager(m, this);
    m_routeManager = new RouteManager(m, this);
    m_locationIndicatorManager = new LocationIndicatorManager(m, this);

    m_locationOverlay = new QLabel(this);
    m_locationOverlay->setFixedSize(LOCATION_OVERLAY_SIZE, LOCATION_OVERLAY_SIZE);
    m_locationOverlay->setStyleSheet(QStringLiteral("background: transparent;"));
    m_locationOverlay->hide();
    m_locationIndicatorManager->setOverlayWidget(m_locationOverlay);
}

void MapContainer::onFollowStep() {
    if (!m_locationIndicatorManager)
        return;
    if (m_locationIndicatorManager->mode() != LocationIndicatorManager::LocationMode::Fixed)
        return;
    if (m_locationIndicatorManager->isFollowingPaused())
        return;

    auto coord = map()->coordinate();
    double lat = CameraMath::lerp(coord.first, m_followTargetLat, FOLLOW_LERP_FACTOR);
    double lon = CameraMath::lerp(coord.second, m_followTargetLon, FOLLOW_LERP_FACTOR);

    map()->setCoordinate(QMapLibre::Coordinate(lat, lon));
    m_lastLat = lat;
    m_lastLon = lon;
}
