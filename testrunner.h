#pragma once

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QImage>
#include <QVector>
#include <QStringList>
#include <QElapsedTimer>
#include "mapannotation.h"
#include "maproutesegment.h"
#include "locationindicatormanager.h"

class MapContainer;

/**
 * @brief 自动化测试运行器
 *
 * 在设备上自动运行地图所有功能的测试序列，通过 qDebug 输出详细日志，
 * 可在 Android 上通过 adb logcat -s "Qt:*" 查看。
 *
 * 测试覆盖范围（共 37 个步骤）：
 *   1. 地图基础操作（12 步）：
 *      - 缩放：Z8→Z6（缩小）、Z6→Z12（放大）、Z12→Z8（复位）
 *      - 旋转：0°→45°、45°→90°、90°→0°
 *      - 平移：Algiers→上海、上海→北京、北京→Algiers
 *      - 倾斜：0°→30°、30°→60°、60°→0°
 *   2. 标注功能（8 步）：单点/批量添加、显示/隐藏、单点删除、全部清除
 *   3. 线路功能（8 步）：单条/批量添加、显示/隐藏、单段删除、全部清除
 *   4. 位置指示器（7 步）：设置坐标、显示/隐藏、Fixed/Free 模式、旋转、中心偏移
 *   5. 综合测试（2 步）：标注+线路同时显示、图层独立切换
 *
 * 使用方式：
 *   TestRunner *runner = new TestRunner(mapContainer, parent);
 *   connect(runner, &TestRunner::logMessage, this, &MyClass::onLog);
 *   connect(runner, &TestRunner::allTestsFinished, this, &MyClass::onFinished);
 *   runner->startTests();  // 异步启动，不会阻塞 UI
 *
 * 注意事项：
 *   - 测试依赖 MapContainer 已就绪（mapReady 信号已触发）
 *   - 每步之间有 1500ms 延迟，方便观察效果（可通过 m_stepDelayMs 调整）
 *   - 使用 QTimer::singleShot 实现异步，测试期间 UI 仍可交互
 *   - 在 Android 上运行时，确保 HXGISServer 可访问（127.0.0.1 需改为设备 IP）
 */
class TestRunner : public QObject {
    Q_OBJECT

public:
    explicit TestRunner(MapContainer *mapContainer, QObject *parent = nullptr);

    /**
     * @brief 开始运行所有测试
     */
    void startTests();

    /**
     * @brief 是否正在运行测试
     */
    bool isRunning() const { return m_running; }

signals:
    /**
     * @brief 测试开始
     */
    void testsStarted();

    /**
     * @brief 单个测试用例完成
     * @param name 测试名称
     * @param passed 是否通过
     * @param message 结果消息
     */
    void testCaseFinished(const QString &name, bool passed, const QString &message);

    /**
     * @brief 所有测试完成
     * @param total 总用例数
     * @param passed 通过数
     * @param failed 失败数
     * @param durationMs 耗时（毫秒）
     */
    void allTestsFinished(int total, int passed, int failed, int durationMs);

    /**
     * @brief 日志消息
     * @param level 日志级别: INFO, PASS, FAIL, STEP
     * @param message 日志内容
     */
    void logMessage(const QString &level, const QString &message);

private slots:
    void runNextStep();

private:
    enum TestStep {
        Step_Idle,
        // === 地图基础操作测试 ===
        Step_Map_ZoomOut,
        Step_Map_ZoomIn,
        Step_Map_ZoomReset,
        Step_Map_Rotate45,
        Step_Map_Rotate90,
        Step_Map_RotateReset,
        Step_Map_PanToShanghai,
        Step_Map_PanToBeijing,
        Step_Map_PanReset,
        Step_Map_Tilt30,
        Step_Map_Tilt60,
        Step_Map_TiltReset,
        // === 标注测试 ===
        Step_Annotation_AddSingle,
        Step_Annotation_VerifySingle,
        Step_Annotation_AddBatch,
        Step_Annotation_VerifyBatch,
        Step_Annotation_Hide,
        Step_Annotation_Show,
        Step_Annotation_RemoveSingle,
        Step_Annotation_Clear,
        // === 线路测试 ===
        Step_Route_AddSingle,
        Step_Route_VerifySingle,
        Step_Route_AddBatch,
        Step_Route_VerifyBatch,
        Step_Route_Hide,
        Step_Route_Show,
        Step_Route_RemoveSingle,
        Step_Route_Clear,
        // === 位置测试 ===
        Step_Location_SetPosition,
        Step_Location_Show,
        Step_Location_Hide,
        Step_Location_ModeFixed,
        Step_Location_ModeFree,
        Step_Location_Rotation,
        Step_Location_CenterOffset,
        // === 综合测试 ===
        Step_Combo_AnnotationRoute,
        Step_Combo_LayerToggle,
        // === 结束 ===
        Step_Finished
    };

    void executeStep(TestStep step);
    void nextStep();
    void log(const QString &level, const QString &message);
    void logTestStart(const QString &testName);
    void logTestPass(const QString &message = QString());
    void logTestFail(const QString &message);

    // 测试辅助
    MapAnnotation createAnnotation(const QString &id, double lat, double lon,
                                    const QString &title, const QString &iconName);
    MapRouteSegment createRouteSegment(const QString &id, const QString &routeId,
                                        const QVector<QPair<double, double>> &coords,
                                        const QColor &color, double width, bool dashed);
    QImage createTestIcon(const QColor &color);

    MapContainer *m_mapContainer;
    bool m_running = false;
    TestStep m_currentStep = Step_Idle;
    int m_stepDelayMs = 1500;  // 每步之间的延迟（毫秒）
    int m_passedCount = 0;
    int m_failedCount = 0;
    int m_totalCount = 0;
    QElapsedTimer m_testTimer;
    QString m_currentTestName;

    // 图标缓存
    QMap<QString, QImage> m_testIcons;
};
