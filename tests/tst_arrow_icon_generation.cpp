#include <QtTest/QtTest>
#include "routearrowicon.h"

class TestArrowIconGeneration : public QObject
{
    Q_OBJECT

private slots:
    void testNonNullImage();
    void testImageSize();
    void testRedPixels();
    void testBluePixels();
    void testIconKeyFormat();
    void testDifferentColorsDifferentKeys();
    void testInvalidColorNoCrash();
    void testDefaultBaseSize();
    void testCustomBaseSize();
};

void TestArrowIconGeneration::testNonNullImage()
{
    QImage img = RouteArrowIcon::generateArrowIcon(QColor("#FF0000"));
    QVERIFY(!img.isNull());
}

void TestArrowIconGeneration::testImageSize()
{
    QImage img = RouteArrowIcon::generateArrowIcon(QColor("#FF0000"), 24);
    QCOMPARE(img.width(), 24);
    QCOMPARE(img.height(), 24);
}

void TestArrowIconGeneration::testRedPixels()
{
    QImage img = RouteArrowIcon::generateArrowIcon(QColor("#FF0000"), 24);
    QRgb center = img.pixel(12, 12);
    QVERIFY(qRed(center) > 200);
    QVERIFY(qGreen(center) < 50);
    QVERIFY(qBlue(center) < 50);
}

void TestArrowIconGeneration::testBluePixels()
{
    QImage img = RouteArrowIcon::generateArrowIcon(QColor("#0000FF"), 24);
    QRgb center = img.pixel(12, 12);
    QVERIFY(qRed(center) < 50);
    QVERIFY(qGreen(center) < 50);
    QVERIFY(qBlue(center) > 200);
}

void TestArrowIconGeneration::testIconKeyFormat()
{
    QString key = RouteArrowIcon::iconKeyForColor(QColor("#FF0000"));
    QCOMPARE(key, QStringLiteral("route-arrow-#ff0000"));
}

void TestArrowIconGeneration::testDifferentColorsDifferentKeys()
{
    QString key1 = RouteArrowIcon::iconKeyForColor(QColor("#FF0000"));
    QString key2 = RouteArrowIcon::iconKeyForColor(QColor("#0000FF"));
    QVERIFY(key1 != key2);
}

void TestArrowIconGeneration::testInvalidColorNoCrash()
{
    QImage img = RouteArrowIcon::generateArrowIcon(QColor(), 24);
    QVERIFY(!img.isNull());
    QCOMPARE(img.width(), 24);

    QString key = RouteArrowIcon::iconKeyForColor(QColor());
    QVERIFY(!key.isEmpty());
}

void TestArrowIconGeneration::testDefaultBaseSize()
{
    QImage img = RouteArrowIcon::generateArrowIcon(QColor("#FF0000"));
    QCOMPARE(img.width(), 12);
    QCOMPARE(img.height(), 12);
}

void TestArrowIconGeneration::testCustomBaseSize()
{
    QImage img = RouteArrowIcon::generateArrowIcon(QColor("#FF0000"), 48);
    QCOMPARE(img.width(), 48);
    QCOMPARE(img.height(), 48);
}

QTEST_MAIN(TestArrowIconGeneration)
#include "tst_arrow_icon_generation.moc"
