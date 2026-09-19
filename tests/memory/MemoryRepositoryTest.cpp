#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "memory/MemoryRepository.h"

class MemoryRepositoryTest final : public QObject {
    Q_OBJECT

private slots:
    void persistsConversationMemoryAndProfileAcrossReopen();
    void clearConversationKeepsLongTermMemory();
    void recoversCorruptDatabaseAndPreservesBackup();
};

void MemoryRepositoryTest::persistsConversationMemoryAndProfileAcrossReopen()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString databasePath = QDir(temporaryDirectory.path()).filePath(
        QStringLiteral("memory.sqlite3"));

    {
        lingnest::memory::MemoryRepository repository(databasePath);
        QString errorMessage;
        QVERIFY2(repository.open(&errorMessage), qPrintable(errorMessage));
        QVERIFY2(
            repository.appendConversation(
                QStringLiteral("yuai"),
                lingnest::ai::ChatRole::User,
                QStringLiteral("记住，我喜欢竹子"),
                &errorMessage),
            qPrintable(errorMessage));
        QVERIFY2(
            repository.appendConversation(
                QStringLiteral("yuai"),
                lingnest::ai::ChatRole::Assistant,
                QStringLiteral("好，我记住啦。"),
                &errorMessage),
            qPrintable(errorMessage));
        QVERIFY2(
            repository.upsertMemory(
                QStringLiteral("yuai"),
                QStringLiteral("Preference"),
                QStringLiteral("我喜欢竹子"),
                QStringLiteral("我喜欢竹子"),
                &errorMessage),
            qPrintable(errorMessage));
        QVERIFY2(
            repository.upsertMemory(
                QStringLiteral("yuai"),
                QStringLiteral("Preference"),
                QStringLiteral("我喜欢竹子"),
                QStringLiteral("我非常喜欢竹子"),
                &errorMessage),
            qPrintable(errorMessage));
        QVERIFY2(
            repository.upsertUserProfile(
                QStringLiteral("yuai"),
                QStringLiteral("name"),
                QStringLiteral("小林"),
                &errorMessage),
            qPrintable(errorMessage));
    }

    lingnest::memory::MemoryRepository reopened(databasePath);
    QString errorMessage;
    QVERIFY2(reopened.open(&errorMessage), qPrintable(errorMessage));

    const auto conversation = reopened.recentConversation(
        QStringLiteral("yuai"), 20, &errorMessage);
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
    QCOMPARE(conversation.size(), 2);
    QVERIFY(conversation.at(0).role == lingnest::ai::ChatRole::User);
    QCOMPARE(conversation.at(0).content, QStringLiteral("记住，我喜欢竹子"));
    QVERIFY(conversation.at(1).role == lingnest::ai::ChatRole::Assistant);

    const auto memories = reopened.recentMemories(
        QStringLiteral("yuai"), 20, &errorMessage);
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
    QCOMPARE(memories.size(), 1);
    QCOMPARE(memories.constFirst().type, QStringLiteral("Preference"));
    QCOMPARE(memories.constFirst().content, QStringLiteral("我非常喜欢竹子"));

    const auto profile = reopened.userProfile(
        QStringLiteral("yuai"), &errorMessage);
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
    QCOMPARE(profile.value(QStringLiteral("name")), QStringLiteral("小林"));
}

void MemoryRepositoryTest::clearConversationKeepsLongTermMemory()
{
    lingnest::memory::MemoryRepository repository(QStringLiteral(":memory:"));
    QString errorMessage;
    QVERIFY2(repository.open(&errorMessage), qPrintable(errorMessage));
    QVERIFY(repository.appendConversation(
        QStringLiteral("yuai"),
        lingnest::ai::ChatRole::User,
        QStringLiteral("一条会话"),
        &errorMessage));
    QVERIFY(repository.upsertMemory(
        QStringLiteral("yuai"),
        QStringLiteral("Fact"),
        QStringLiteral("测试事实"),
        QStringLiteral("测试事实"),
        &errorMessage));
    QVERIFY(repository.clearConversation(
        QStringLiteral("yuai"), &errorMessage));

    QCOMPARE(repository.recentConversation(
        QStringLiteral("yuai"), 20, &errorMessage).size(), 0);
    QCOMPARE(repository.recentMemories(
        QStringLiteral("yuai"), 20, &errorMessage).size(), 1);
}

void MemoryRepositoryTest::recoversCorruptDatabaseAndPreservesBackup()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString databasePath = QDir(temporaryDirectory.path()).filePath(
        QStringLiteral("memory.sqlite3"));

    QFile damaged(databasePath);
    QVERIFY(damaged.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray damagedData = QByteArrayLiteral("this is not sqlite");
    QCOMPARE(damaged.write(damagedData), damagedData.size());
    damaged.close();

    lingnest::memory::MemoryRepository repository(databasePath);
    QString recoveryMessage;
    QVERIFY2(
        repository.openWithRecovery(&recoveryMessage),
        qPrintable(recoveryMessage));
    QVERIFY(repository.isOpen());
    QVERIFY(!recoveryMessage.isEmpty());

    const QStringList backups = QDir(temporaryDirectory.path()).entryList(
        {QStringLiteral("memory.sqlite3.corrupt-*")}, QDir::Files);
    QCOMPARE(backups.size(), 1);
    QFile backup(QDir(temporaryDirectory.path()).filePath(backups.constFirst()));
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(backup.readAll(), damagedData);

    QString writeError;
    QVERIFY2(
        repository.appendConversation(
            QStringLiteral("yuai"),
            lingnest::ai::ChatRole::User,
            QStringLiteral("恢复后的消息"),
            &writeError),
        qPrintable(writeError));
}

QTEST_GUILESS_MAIN(MemoryRepositoryTest)

#include "MemoryRepositoryTest.moc"
