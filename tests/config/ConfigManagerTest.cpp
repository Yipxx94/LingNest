#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "config/ConfigManager.h"

namespace {

class ConfigManagerTest final : public QObject {
    Q_OBJECT

private slots:
    void roundTripsPetWindowPlacement();
    void roundTripsNonSecretAISettings();
    void reportsMalformedJson();
    void recoversMalformedJsonAndPreservesBackup();
};

void ConfigManagerTest::roundTripsPetWindowPlacement()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString configPath =
        QDir(temporaryDirectory.path()).filePath(QStringLiteral("nested/config.json"));
    lingnest::config::ConfigManager writer(configPath);

    QString errorMessage;
    QVERIFY2(writer.load(&errorMessage), qPrintable(errorMessage));
    QVERIFY(!writer.petWindowPlacement().has_value());

    lingnest::config::WindowPlacement expected;
    expected.screenName = QStringLiteral("DISPLAY-2");
    expected.position = QPoint(-1400, 640);
    expected.relativeX = 0.25;
    expected.relativeY = 0.75;
    expected.hasRelativePosition = true;
    QVERIFY2(writer.savePetWindowPlacement(expected, &errorMessage), qPrintable(errorMessage));

    lingnest::config::ConfigManager reader(configPath);
    QVERIFY2(reader.load(&errorMessage), qPrintable(errorMessage));
    const auto actual = reader.petWindowPlacement();

    QVERIFY(actual.has_value());
    QCOMPARE(actual->screenName, expected.screenName);
    QCOMPARE(actual->position, expected.position);
    QCOMPARE(actual->relativeX, expected.relativeX);
    QCOMPARE(actual->relativeY, expected.relativeY);
    QVERIFY(actual->hasRelativePosition);
}

void ConfigManagerTest::roundTripsNonSecretAISettings()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString configPath =
        QDir(temporaryDirectory.path()).filePath(QStringLiteral("config.json"));
    lingnest::config::ConfigManager writer(configPath);
    QString errorMessage;
    QVERIFY2(writer.load(&errorMessage), qPrintable(errorMessage));

    lingnest::config::AIProviderConfig expected;
    expected.baseUrl = QStringLiteral("https://example.test/openai/v1");
    expected.model = QStringLiteral("example-model");
    expected.temperature = 1.25;
    expected.maxTokens = 777;
    expected.timeoutMs = 45000;
    QVERIFY2(writer.saveAIProviderConfig(expected, &errorMessage), qPrintable(errorMessage));

    lingnest::config::ConfigManager reader(configPath);
    QVERIFY2(reader.load(&errorMessage), qPrintable(errorMessage));
    const lingnest::config::AIProviderConfig actual = reader.aiProviderConfig();
    QCOMPARE(actual.baseUrl, expected.baseUrl);
    QCOMPARE(actual.model, expected.model);
    QCOMPARE(actual.temperature, expected.temperature);
    QCOMPARE(actual.maxTokens, expected.maxTokens);
    QCOMPARE(actual.timeoutMs, expected.timeoutMs);

    QFile file(configPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray storedData = file.readAll();
    QVERIFY(!storedData.contains("apiKey"));
    QVERIFY(!storedData.contains("Authorization"));
}

void ConfigManagerTest::reportsMalformedJson()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString configPath =
        QDir(temporaryDirectory.path()).filePath(QStringLiteral("config.json"));
    QFile file(configPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(file.write(QByteArrayLiteral("{")), 1);
    file.close();

    lingnest::config::ConfigManager manager(configPath);
    QString errorMessage;
    QVERIFY(!manager.load(&errorMessage));
    QVERIFY(!errorMessage.isEmpty());
}

void ConfigManagerTest::recoversMalformedJsonAndPreservesBackup()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString configPath =
        QDir(temporaryDirectory.path()).filePath(QStringLiteral("config.json"));
    QFile file(configPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray invalidJson = QByteArrayLiteral("{private-but-invalid");
    QCOMPARE(file.write(invalidJson), invalidJson.size());
    file.close();

    lingnest::config::ConfigManager manager(configPath);
    QString recoveryMessage;
    QVERIFY2(manager.loadWithRecovery(&recoveryMessage), qPrintable(recoveryMessage));
    QVERIFY(!recoveryMessage.isEmpty());
    QVERIFY(!manager.petWindowPlacement().has_value());

    const QStringList backups = QDir(temporaryDirectory.path()).entryList(
        {QStringLiteral("config.json.corrupt-*")}, QDir::Files);
    QCOMPARE(backups.size(), 1);

    QFile backup(QDir(temporaryDirectory.path()).filePath(backups.constFirst()));
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(backup.readAll(), invalidJson);

    QFile recovered(configPath);
    QVERIFY(recovered.open(QIODevice::ReadOnly));
    const QJsonDocument recoveredDocument = QJsonDocument::fromJson(recovered.readAll());
    QVERIFY(recoveredDocument.isObject());
    QCOMPARE(
        recoveredDocument.object().value(QStringLiteral("schemaVersion")).toInt(),
        2);
}

} // namespace

QTEST_APPLESS_MAIN(ConfigManagerTest)

#include "ConfigManagerTest.moc"
