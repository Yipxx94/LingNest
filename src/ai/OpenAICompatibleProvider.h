#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QUrl>

#include "ai/IAIProvider.h"

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace lingnest::ai {

struct OpenAICompatibleSettings {
    QUrl baseUrl {QStringLiteral("https://api.openai.com/v1")};
    QString apiKey;
    QString model;
    double temperature {0.7};
    int maxTokens {512};
    int timeoutMs {30000};
};

class OpenAICompatibleProvider final : public IAIProvider {
    Q_OBJECT

public:
    explicit OpenAICompatibleProvider(
        OpenAICompatibleSettings settings = {},
        QObject* parent = nullptr);

    [[nodiscard]] const OpenAICompatibleSettings& settings() const noexcept;
    void setSettings(OpenAICompatibleSettings settings);

    quint64 sendChat(const AIChatRequest& request) override;
    void cancel(quint64 requestId) override;

private:
    struct PendingRequest {
        QNetworkReply* reply {nullptr};
        QTimer* timeoutTimer {nullptr};
    };

    [[nodiscard]] QString validateSettings() const;
    [[nodiscard]] QUrl chatCompletionsUrl() const;
    void finishNetworkRequest(quint64 requestId);
    void timeoutRequest(quint64 requestId);
    void complete(quint64 requestId, AIChatResult result);

    OpenAICompatibleSettings m_settings;
    QNetworkAccessManager* m_networkAccessManager {nullptr};
    QHash<quint64, PendingRequest> m_pendingRequests;
    QSet<quint64> m_deferredRequests;
    quint64 m_nextRequestId {1};
};

} // namespace lingnest::ai
