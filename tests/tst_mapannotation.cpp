/**
 * @file tst_mapannotation.cpp
 * @brief MapAnnotation 数据结构单元测试
 *
 * 测试覆盖：
 *   - 默认值（零初始化）
 *   - 有效标注验证
 *   - 无效纬度边界
 *   - 无效经度边界
 *   - 空 ID 验证
 *   - 边界值（±90°, ±180°）
 *   - 空 title/iconName 不影响有效性
 */

#include <QtTest>
#include "mapannotation.h"

class MapAnnotationTest : public QObject {
    Q_OBJECT

private slots:
    /** 默认构造的标注：id 为空，坐标为 0.0，应为无效 */
    void testDefaultValues()
    {
        MapAnnotation ann;
        QCOMPARE(ann.id, QString());
        QCOMPARE(ann.latitude, 0.0);
        QCOMPARE(ann.longitude, 0.0);
        QCOMPARE(ann.title, QString());
        QCOMPARE(ann.iconName, QString());
        // 空 id → 无效
        QVERIFY(!ann.isValid());
    }

    /** 填入合法数据后应为有效 */
    void testValidAnnotation()
    {
        MapAnnotation ann;
        ann.id = QStringLiteral("ann-001");
        ann.latitude = 39.9042;
        ann.longitude = 116.4074;
        ann.title = QStringLiteral("北京天安门");
        ann.iconName = QStringLiteral("marker");
        QVERIFY(ann.isValid());
    }

    /** 纬度超出 [-90, 90] 应为无效 */
    void testInvalidLatitude()
    {
        MapAnnotation ann;
        ann.id = QStringLiteral("ann-lat");
        ann.longitude = 0.0;

        ann.latitude = 91.0;
        QVERIFY(!ann.isValid());

        ann.latitude = -91.0;
        QVERIFY(!ann.isValid());

        ann.latitude = 200.0;
        QVERIFY(!ann.isValid());
    }

    /** 经度超出 [-180, 180] 应为无效 */
    void testInvalidLongitude()
    {
        MapAnnotation ann;
        ann.id = QStringLiteral("ann-lon");
        ann.latitude = 0.0;

        ann.longitude = 181.0;
        QVERIFY(!ann.isValid());

        ann.longitude = -181.0;
        QVERIFY(!ann.isValid());

        ann.longitude = 360.0;
        QVERIFY(!ann.isValid());
    }

    /** id 为空时无论坐标如何都应无效 */
    void testEmptyId()
    {
        MapAnnotation ann;
        ann.latitude = 39.9;
        ann.longitude = 116.4;
        ann.title = QStringLiteral("无名标注");
        QVERIFY(!ann.isValid());
    }

    /** 纬度 ±90、经度 ±180 为有效边界 */
    void testBoundaryValues()
    {
        MapAnnotation ann;
        ann.id = QStringLiteral("boundary");

        // 北极点
        ann.latitude = 90.0;
        ann.longitude = 0.0;
        QVERIFY(ann.isValid());

        // 南极点
        ann.latitude = -90.0;
        ann.longitude = 0.0;
        QVERIFY(ann.isValid());

        // 国际日期变更线 (东)
        ann.latitude = 0.0;
        ann.longitude = 180.0;
        QVERIFY(ann.isValid());

        // 国际日期变更线 (西)
        ann.latitude = 0.0;
        ann.longitude = -180.0;
        QVERIFY(ann.isValid());

        // 四角极限
        ann.latitude = 90.0;
        ann.longitude = 180.0;
        QVERIFY(ann.isValid());

        ann.latitude = -90.0;
        ann.longitude = -180.0;
        QVERIFY(ann.isValid());
    }

    /** title 和 iconName 为空不影响有效性 */
    void testEmptyTitleAndIcon()
    {
        MapAnnotation ann;
        ann.id = QStringLiteral("bare");
        ann.latitude = 35.68;
        ann.longitude = 139.69;
        // title 和 iconName 保持默认空字符串
        QVERIFY(ann.isValid());
        QVERIFY(ann.title.isEmpty());
        QVERIFY(ann.iconName.isEmpty());
    }
};

QTEST_MAIN(MapAnnotationTest)
#include "tst_mapannotation.moc"
