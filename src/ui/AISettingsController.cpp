#include "ui/AISettingsController.h"

#include <cmath>

#include <QUrl>
#include <QtGlobal>

#include "ai/OpenAICompatibleProvider.h"
#include "config/ISecretStore.h"

namespace lingnest::ui {

AISettingsController::AISettingsController(
    config::ConfigManager* configManager,
    config::ISecretStore* secretStore,
    ai::OpenAICompatibleProvider* provider,
    bool loadSecret,
    QObject* parent)
    : QObject(parent)
    , m_configManager(configManager)
    , m_secretStore(secretStore)
    , m_provider(provider)
{
    Q_ASSERT(m_configManager != nullptr);
    Q_ASSERT(m_secretStore != nullptr);
    Q_ASSERT(m_provider != nullptr);

    m_config = m_configManager->aiProviderConfig();
    if (loadSecret) {
        QString errorMessage;
        const std::optional<QString> secret = m_secretStore->readSecret(
            credentialTarget(), &errorMessage);
        if (secret.has_value()) {
            m_apiKey = *secret;
        } else if (!errorMessage.isEmpty()) {
            setStatus(errorMessage, true);
        }
    }
    updateProvider();
}

QString AISettingsController::credentialTarget()
{
    return QStringLiteral("LingNest/OpenAICompatible/default");
}

QString AISettingsController::baseUrl() const
{
    return m_config.baseUrl;
}

QString AISettingsController::model() const
{
    return m_config.model;
}

double AISettingsController::temperature() const noexcept
{
    return m_config.temperature;
}

int AISettingsController::maxTokens() const noexcept
{
    return m_config.maxTokens;
}

int AISettingsController::timeoutSeconds() const noexcept
{
    return qMax(1, m_config.timeoutMs / 1000);
}

bool AISettingsController::isApiKeyConfigured() const noexcept
{
    return !m_apiKey.isEmpty();
}

QString AISettingsController::statusMessage() const
{
    return m_statusMessage;
}

bool AISettingsController::isStatusError() const noexcept
{
    return m_statusError;
}

void AISettingsController::save(
    const QString& baseUrl,
    const QString& model,
    double temperature,
    int maxTokens,
    int timeoutSeconds,
    const QString& newApiKey)
{
    const QString normalizedBaseUrl = baseUrl.trimmed();
    const QUrl parsedBaseUrl(normalizedBaseUrl);
    if (!parsedBaseUrl.isValid()
        || (parsedBaseUrl.scheme() != QStringLiteral("http")
            && parsedBaseUrl.scheme() != QStringLiteral("https"))
        || parsedBaseUrl.host().isEmpty()) {
        setStatus(QStringLiteral("Base URL 必须是有效的 HTTP 或 HTTPS 地址。"), true);
        return;
    }
    if (model.trimmed().isEmpty()) {
        setStatus(QStringLiteral("模型名称不能为空。"), true);
        return;
    }
    if (!std::isfinite(temperature) || temperature < 0.0 || temperature > 2.0) {
        setStatus(QStringLiteral("Temperature 必须在 0 到 2 之间。"), true);
        return;
    }
    if (maxTokens <= 0) {
        setStatus(QStringLiteral("Max Tokens 必须大于 0。"), true);
        return;
    }
    if (timeoutSeconds <= 0 || timeoutSeconds > 600) {
        setStatus(QStringLiteral("超时时间必须在 1 到 600 秒之间。"), true);
        return;
    }

    config::AIProviderConfig nextConfig;
    nextConfig.baseUrl = normalizedBaseUrl;
    nextConfig.model = model.trimmed();
    nextConfig.temperature = temperature;
    nextConfig.maxTokens = maxTokens;
    nextConfig.timeoutMs = timeoutSeconds * 1000;

    QString errorMessage;
    if (!m_configManager->saveAIProviderConfig(nextConfig, &errorMessage)) {
        setStatus(QStringLiteral("保存 AI 配置失败：%1").arg(errorMessage), true);
        return;
    }

    const QString normalizedApiKey = newApiKey.trimmed();
    if (!normalizedApiKey.isEmpty()) {
        if (!m_secretStore->writeSecret(
                credentialTarget(), normalizedApiKey, &errorMessage)) {
            m_config = nextConfig;
            updateProvider();
            emit settingsChanged();
            setStatus(errorMessage, true);
            return;
        }

        const bool wasConfigured = isApiKeyConfigured();
        m_apiKey = normalizedApiKey;
        if (!wasConfigured) {
            emit apiKeyConfiguredChanged();
        }
    }

    m_config = nextConfig;
    updateProvider();
    emit settingsChanged();
    setStatus(
        isApiKeyConfigured()
            ? QStringLiteral("AI 设置已保存，API Key 安全存储在 Windows 凭据管理器中。")
            : QStringLiteral("参数已保存；还需要填写 API Key 才能聊天。"),
        false);
}

void AISettingsController::clearApiKey()
{
    QString errorMessage;
    if (!m_secretStore->removeSecret(credentialTarget(), &errorMessage)) {
        setStatus(errorMessage, true);
        return;
    }

    const bool wasConfigured = isApiKeyConfigured();
    m_apiKey.clear();
    updateProvider();
    if (wasConfigured) {
        emit apiKeyConfiguredChanged();
    }
    setStatus(QStringLiteral("已从 Windows 凭据管理器删除 API Key。"), false);
}

void AISettingsController::updateProvider()
{
    ai::OpenAICompatibleSettings settings;
    settings.baseUrl = QUrl(m_config.baseUrl);
    settings.apiKey = m_apiKey;
    settings.model = m_config.model;
    settings.temperature = m_config.temperature;
    settings.maxTokens = m_config.maxTokens;
    settings.timeoutMs = m_config.timeoutMs;
    m_provider->setSettings(std::move(settings));
}

void AISettingsController::setStatus(QString message, bool isError)
{
    if (m_statusMessage == message && m_statusError == isError) {
        return;
    }
    m_statusMessage = std::move(message);
    m_statusError = isError;
    emit statusChanged();
}

} // namespace lingnest::ui
