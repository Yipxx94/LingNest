#pragma once

#include <QObject>
#include <QString>

#include "config/ConfigManager.h"

namespace lingnest::ai {
class OpenAICompatibleProvider;
}

namespace lingnest::config {
class ISecretStore;
}

namespace lingnest::ui {

class AISettingsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString baseUrl READ baseUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString model READ model NOTIFY settingsChanged)
    Q_PROPERTY(double temperature READ temperature NOTIFY settingsChanged)
    Q_PROPERTY(int maxTokens READ maxTokens NOTIFY settingsChanged)
    Q_PROPERTY(int timeoutSeconds READ timeoutSeconds NOTIFY settingsChanged)
    Q_PROPERTY(bool apiKeyConfigured READ isApiKeyConfigured NOTIFY apiKeyConfiguredChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool statusError READ isStatusError NOTIFY statusChanged)

public:
    AISettingsController(
        config::ConfigManager* configManager,
        config::ISecretStore* secretStore,
        ai::OpenAICompatibleProvider* provider,
        bool loadSecret = true,
        QObject* parent = nullptr);

    [[nodiscard]] static QString credentialTarget();
    [[nodiscard]] QString baseUrl() const;
    [[nodiscard]] QString model() const;
    [[nodiscard]] double temperature() const noexcept;
    [[nodiscard]] int maxTokens() const noexcept;
    [[nodiscard]] int timeoutSeconds() const noexcept;
    [[nodiscard]] bool isApiKeyConfigured() const noexcept;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] bool isStatusError() const noexcept;

    Q_INVOKABLE void save(
        const QString& baseUrl,
        const QString& model,
        double temperature,
        int maxTokens,
        int timeoutSeconds,
        const QString& newApiKey);
    Q_INVOKABLE void clearApiKey();

signals:
    void settingsChanged();
    void apiKeyConfiguredChanged();
    void statusChanged();

private:
    void updateProvider();
    void setStatus(QString message, bool isError);

    config::ConfigManager* m_configManager {nullptr};
    config::ISecretStore* m_secretStore {nullptr};
    ai::OpenAICompatibleProvider* m_provider {nullptr};
    config::AIProviderConfig m_config;
    QString m_apiKey;
    QString m_statusMessage;
    bool m_statusError {false};
};

} // namespace lingnest::ui
