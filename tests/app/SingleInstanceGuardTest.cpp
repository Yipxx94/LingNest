#include <QtTest>

#include <memory>

#include <QDir>
#include <QTemporaryDir>

#include "app/SingleInstanceGuard.h"

class SingleInstanceGuardTest final : public QObject {
    Q_OBJECT

private slots:
    void preventsConcurrentInstanceAndReleasesLock();
};

void SingleInstanceGuardTest::preventsConcurrentInstanceAndReleasesLock()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString lockFilePath = QDir(temporaryDirectory.path()).filePath(
        QStringLiteral("lingnest-test.lock"));

    {
        auto primary = std::make_unique<lingnest::app::SingleInstanceGuard>(
            lockFilePath);
        QVERIFY(primary->tryAcquire());
        QVERIFY(primary->isAcquired());
        QVERIFY(primary->tryAcquire());

        lingnest::app::SingleInstanceGuard duplicate(lockFilePath);
        QVERIFY(!duplicate.tryAcquire());
        QVERIFY(!duplicate.isAcquired());
    }

    lingnest::app::SingleInstanceGuard relaunched(lockFilePath);
    QVERIFY(relaunched.tryAcquire());
    QVERIFY(relaunched.isAcquired());
}

QTEST_GUILESS_MAIN(SingleInstanceGuardTest)

#include "SingleInstanceGuardTest.moc"
