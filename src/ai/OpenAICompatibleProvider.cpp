#include "ai/OpenAICompatibleProvider.h"

#include <utility>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QVariant>

namespace lingnest::ai {
namespace {

QString apiRole(ChatRole role)
{
    switch (role) {
    case ChatRole::System:
        return QStringLiteral("system");
    case ChatRole::User:
        return QStringLiteral("user");
    case ChatRole::Assistant:
        return QStringLiteral("assistant");
    }
    return QStringLiteral("user");
}

AIChatResult failure(
    AIErrorCode errorCode,
    QString message,
    int httpStatus = 0)
{
    AIChatResult result;
    result.errorCode = errorCode;
    result.errorMessage = std::move(message);
    result.httpStatus = httpStatus;
    return result;
}

QString boundedServerMessage(QString message)
{
    message = message.simplified();
    if (message.size() > 400) {
        message = message.left(400) + QStringLiteral("…");
    }
    return message;
}

QString redactSecret(QString message, const QString& secret)
{
    const QString normalizedSecret = secret.trimmed();
    if (!normalizedSecret.isEmpty()) {
        message.replace(
            normalizedSecret,
            QStringLiteral("[redacted]"),
            Qt::CaseSensitive);
    }
    return boundedServerMessage(std::move(message));
}

QString apiErrorMessage(const QJsonObject& root, const QString& secret)
{
    const QJsonValue errorValue = root.value(QStringLiteral("error"));
    if (errorValue.isObject()) {
        return redactSecret(
            errorValue.toObject().value(QStringLiteral("message")).toString(),
            secret);
    }
    if (errorValue.isString()) {
        return redactSecret(errorValue.toString(), secret);
    }
    return {};
}

} // namespace

OpenAICompatibleProvider::OpenAICompatibleProvider(
    OpenAICompatibleSettings settings,
    QObject* parent)
    : IAIProvider(parent)
    , m_settings(std::move(settings))
    , m_networkAccessManager(new QNetworkAccessManager(this))
{
}

const OpenAICompatibleSettings& OpenAICompatibleProvider::settings() const noexcept
{
    return m_settings;
}

void OpenAICompatibleProvider::setSettings(OpenAICompatibleSettings settings)
{
    m_settings = std::move(settings);
}

quint64 OpenAICompatibleProvider::sendChat(const AIChatRequest& request)
{
    const quint64 requestId = m_nextRequestId++;
    const QString settingsError = validateSettings();
    if (!settingsError.isEmpty()) {
        m_deferredRequests.insert(requestId);
        QTimer::singleShot(0, this, [this, requestId, settingsError]() {
            if (!m_deferredRequests.remove(requestId)) {
                return;
            }
            emit chatFinished(
                requestId,
                failure(AIErrorCode::Configuration, settingsError));
        });
        return requestId;
    }

    if (request.messages.isEmpty()) {
        m_deferredRequests.insert(requestId);
        QTimer::singleShot(0, this, [this, requestId]() {
            if (!m_deferredRequests.remove(requestId)) {
                return;
            }
            emit chatFinished(
                requestId,
                failure(
                    AIErrorCode::Configuration,
                    QStringLiteral("没有可发送的对话内容。")));
        });
        return requestId;
    }

    QJsonArray messages;
    for (const AIChatMessage& message : request.messages) {
        messages.append(QJsonObject {
            {QStringLiteral("role"), apiRole(message.role)},
            {QStringLiteral("content"), message.content}
        });
    }

    QJsonObject body;
    body.insert(QStringLiteral("model"), m_settings.model);
    body.insert(QStringLiteral("messages"), messages);
    body.insert(QStringLiteral("temperature"), m_settings.temperature);
    body.insert(QStringLiteral("max_tokens"), m_settings.maxTokens);

    QNetworkRequest networkRequest(chatCompletionsUrl());
    networkRequest.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral("application/json"));
    networkRequest.setRawHeader(
        QByteArrayLiteral("Accept"),
        QByteArrayLiteral("application/json"));

    QByteArray authorization = QByteArrayLiteral("Bearer ");
    authorization.append(m_settings.apiKey.toUtf8());
    networkRequest.setRawHeader(QByteArrayLiteral("Authorization"), authorization);
    authorization.fill('\0');

    const QByteArray requestBody = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = m_networkAccessManager->post(networkRequest, requestBody);
    auto* timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);

    m_pendingRequests.insert(requestId, {reply, timeoutTimer});
    connect(reply, &QNetworkReply::finished, this, [this, requestId]() {
        finishNetworkRequest(requestId);
    });
    connect(timeoutTimer, &QTimer::timeout, this, [this, requestId]() {
        timeoutRequest(requestId);
    });
    timeoutTimer->start(m_settings.timeoutMs);
    return requestId;
}

void OpenAICompatibleProvider::cancel(quint64 requestId)
{
    if (m_deferredRequests.remove(requestId)) {
        QTimer::singleShot(0, this, [this, requestId]() {
            emit chatFinished(
                requestId,
                failure(AIErrorCode::Cancelled, QStringLiteral("请求已取消。")));
        });
        return;
    }

    const auto iterator = m_pendingRequests.find(requestId);
    if (iterator == m_pendingRequests.end()) {
        return;
    }

    const PendingRequest pending = iterator.value();
    m_pendingRequests.erase(iterator);
    pending.timeoutTimer->stop();
    pending.timeoutTimer->deleteLater();
    disconnect(pending.reply, nullptr, this, nullptr);
    pending.reply->abort();
    pending.reply->deleteLater();

    QTimer::singleShot(0, this, [this, requestId]() {
        emit chatFinished(
            requestId,
            failure(AIErrorCode::Cancelled, QStringLiteral("请求已取消。")));
    });
}

QString OpenAICompatibleProvider::validateSettings() const
{
    if (!m_settings.baseUrl.isValid()
        || (m_settings.baseUrl.scheme() != QStringLiteral("http")
            && m_settings.baseUrl.scheme() != QStringLiteral("https"))
        || m_settings.baseUrl.host().isEmpty()) {
        return QStringLiteral("AI Base URL 无效，请在设置中填写 HTTP 或 HTTPS 地址。");
    }
    if (m_settings.model.trimmed().isEmpty()) {
        return QStringLiteral("尚未配置 AI 模型，请在设置中填写模型名称。");
    }
    if (m_settings.apiKey.trimmed().isEmpty()) {
        return QStringLiteral("尚未配置 API Key，请在设置中安全保存密钥。");
    }
    if (m_settings.temperature < 0.0 || m_settings.temperature > 2.0) {
        return QStringLiteral("Temperature 必须在 0 到 2 之间。");
    }
    if (m_settings.maxTokens <= 0) {
        return QStringLiteral("Max Tokens 必须大于 0。");
    }
    if (m_settings.timeoutMs <= 0) {
        return QStringLiteral("请求超时时间必须大于 0。");
    }
    return {};
}

QUrl OpenAICompatibleProvider::chatCompletionsUrl() const
{
    QUrl endpoint = m_settings.baseUrl;
    endpoint.setQuery({});
    endpoint.setFragment({});

    QString path = endpoint.path();
    while (path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
    }
    if (!path.endsWith(QStringLiteral("/chat/completions"))) {
        path += QStringLiteral("/chat/completions");
    }
    endpoint.setPath(path);
    return endpoint;
}

void OpenAICompatibleProvider::finishNetworkRequest(quint64 requestId)
{
    const auto iterator = m_pendingRequests.find(requestId);
    if (iterator == m_pendingRequests.end()) {
        return;
    }

    const PendingRequest pending = iterator.value();
    m_pendingRequests.erase(iterator);
    pending.timeoutTimer->stop();
    pending.timeoutTimer->deleteLater();

    QNetworkReply* reply = pending.reply;
    const int httpStatus = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray responseBody = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    reply->deleteLater();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(responseBody, &parseError);
    const QJsonObject root = document.isObject() ? document.object() : QJsonObject {};
    const QString serverMessage = apiErrorMessage(root, m_settings.apiKey);

    if (httpStatus < 200 || httpStatus >= 300) {
        if (httpStatus == 0 && networkError != QNetworkReply::NoError) {
            complete(
                requestId,
                failure(
                    AIErrorCode::Network,
                    QStringLiteral("无法连接 AI 服务：%1")
                        .arg(redactSecret(networkErrorText, m_settings.apiKey))));
            return;
        }

        const QString message = serverMessage.isEmpty()
            ? QStringLiteral("AI 服务返回 HTTP %1。").arg(httpStatus)
            : QStringLiteral("AI 服务返回 HTTP %1：%2")
                  .arg(httpStatus)
                  .arg(serverMessage);
        complete(
            requestId,
            failure(
                serverMessage.isEmpty() ? AIErrorCode::Http : AIErrorCode::Api,
                message,
                httpStatus));
        return;
    }

    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        complete(
            requestId,
            failure(
                AIErrorCode::Json,
                QStringLiteral("AI 服务返回了无法解析的数据。"),
                httpStatus));
        return;
    }

    if (!serverMessage.isEmpty()) {
        complete(
            requestId,
            failure(
                AIErrorCode::Api,
                QStringLiteral("AI 服务报告错误：%1").arg(serverMessage),
                httpStatus));
        return;
    }

    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        complete(
            requestId,
            failure(
                AIErrorCode::Json,
                QStringLiteral("AI 服务响应中没有回复内容。"),
                httpStatus));
        return;
    }

    const QJsonObject message = choices.at(0)
                                    .toObject()
                                    .value(QStringLiteral("message"))
                                    .toObject();
    const QJsonValue contentValue = message.value(QStringLiteral("content"));
    if (!contentValue.isString() || contentValue.toString().trimmed().isEmpty()) {
        complete(
            requestId,
            failure(
                AIErrorCode::Json,
                QStringLiteral("AI 服务响应中的回复格式不正确。"),
                httpStatus));
        return;
    }

    AIChatResult result;
    result.success = true;
    result.content = contentValue.toString().trimmed();
    result.httpStatus = httpStatus;
    complete(requestId, std::move(result));
}

void OpenAICompatibleProvider::timeoutRequest(quint64 requestId)
{
    const auto iterator = m_pendingRequests.find(requestId);
    if (iterator == m_pendingRequests.end()) {
        return;
    }

    const PendingRequest pending = iterator.value();
    m_pendingRequests.erase(iterator);
    pending.timeoutTimer->deleteLater();
    disconnect(pending.reply, nullptr, this, nullptr);
    pending.reply->abort();
    pending.reply->deleteLater();

    complete(
        requestId,
        failure(
            AIErrorCode::Timeout,
            QStringLiteral("AI 请求超时，请稍后重试或增大超时时间。")));
}

void OpenAICompatibleProvider::complete(quint64 requestId, AIChatResult result)
{
    emit chatFinished(requestId, std::move(result));
}

} // namespace lingnest::ai
