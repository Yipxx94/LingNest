#include <QtTest>

#include <QDir>
#include <QFile>
#include <QHash>
#include <QTemporaryDir>

#include "ai/OpenAICompatibleProvider.h"
#include "config/ConfigManager.h"
#include "config/ISecretStore.h"
#include "ui/AISettingsController.h"

namespace {

class InMemorySecretStore final : public lingnest::config::ISecretStore {
public:
    std::optional<QString> readSecret(
        const QString& target,
        QString*) override
    {
        const auto iterator = secrets.constFind(target);
        if (iterator == secrets.constEnd()) {
            return std::nullopt;
        }
        return iterator.value();
    }

    bool writeSecret(
        const QString& target,
        const QString& secret,
        QString*) override
    {
        secrets.insert(target, secret);
        return true;
    }

    bool removeSecret(const QString& target, QString*) override
    {
        secrets.remove(target);
        return true;
    }

    QHash<QString, QString> secrets;
};

class AISettingsControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void keepsSecretOutOfJsonAndRefreshesProvider();
    void rejectsInvalidValuesWithoutChangingConfig();
};

void AISettingsControllerTest::keepsSecretOutOfJsonAndRefreshesProvider()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString configPath =
        QDir(temporaryDirectory.path()).filePath(QStringLiteral("config.json"));
    lingnest::config::ConfigManager configManager(configPath);
    QString errorMessage;
    QVERIFY2(configManager.load(&errorMessage), qPrintable(errorMessage));

    InMemorySecretStore secretStore;
    secretStore.secrets.insert(
        lingnest::ui::AISettingsController::credentialTarget(),
        QStringLiteral("old-secret"));
    lingnest::ai::OpenAICompatibleProvider provider;
    lingnest::ui::AISettingsController controller(
        &configManager, &secretStore, &provider);

    QVERIFY(controller.isApiKeyConfigured());
    QCOMPARE(provider.settings().apiKey, QStringLiteral("old-secret"));

    controller.save(
        QStringLiteral("https://example.test/api/v1/"),
        QStringLiteral("friendly-model"),
        0.4,
        900,
        42,
        QStringLiteral("new-secret"));

    QVERIFY(!controller.isStatusError());
    QCOMPARE(controller.model(), QStringLiteral("friendly-model"));
    QCOMPARE(controller.maxTokens(), 900);
    QCOMPARE(controller.timeoutSeconds(), 42);
    QCOMPARE(provider.settings().baseUrl.toString(), QStringLiteral("https://example.test/api/v1/"));
    QCOMPARE(provider.settings().model, QStringLiteral("friendly-model"));
    QCOMPARE(provider.settings().temperature, 0.4);
    QCOMPARE(provider.settings().maxTokens, 900);
    QCOMPARE(provider.settings().timeoutMs, 42000);
    QCOMPARE(provider.settings().apiKey, QStringLiteral("new-secret"));
    QCOMPARE(
        secretStore.secrets.value(
            lingnest::ui::AISettingsController::credentialTarget()),
        QStringLiteral("new-secret"));

    QFile configFile(configPath);
    QVERIFY(configFile.open(QIODevice::ReadOnly));
    const QByteArray configData = configFile.readAll();
    QVERIFY(!configData.contains("old-secret"));
    QVERIFY(!configData.contains("new-secret"));
    QVERIFY(!configData.contains("apiKey"));

    controller.clearApiKey();
    QVERIFY(!controller.isApiKeyConfigured());
    QVERIFY(provider.settings().apiKey.isEmpty());
    QVERIFY(!secretStore.secrets.contains(
        lingnest::ui::AISettingsController::credentialTarget()));
}

void AISettingsControllerTest::rejectsInvalidValuesWithoutChangingConfig()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString configPath =
        QDir(temporaryDirectory.path()).filePath(QStringLiteral("config.json"));
    lingnest::config::ConfigManager configManager(configPath);
    QString errorMessage;
    QVERIFY2(configManager.load(&errorMessage), qPrintable(errorMessage));

    InMemorySecretStore secretStore;
    lingnest::ai::OpenAICompatibleProvider provider;
    lingnest::ui::AISettingsController controller(
        &configManager, &secretStore, &provider);

    controller.save(
        QStringLiteral("not-a-url"),
        QStringLiteral("model"),
        0.7,
        512,
        30,
        QStringLiteral("must-not-be-written"));

    QVERIFY(controller.isStatusError());
    QVERIFY(!QFile::exists(configPath));
    QVERIFY(secretStore.secrets.isEmpty());
    QVERIFY(provider.settings().model.isEmpty());
}

} // namespace

QTEST_GUILESS_MAIN(AISettingsControllerTest)

#include "AISettingsControllerTest.moc"
