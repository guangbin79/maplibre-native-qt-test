#include <QtTest>

class PlaceholderTest : public QObject {
    Q_OBJECT
private slots:
    void testPass() { QVERIFY(true); }
};

QTEST_MAIN(PlaceholderTest)
#include "tst_placeholder.moc"
