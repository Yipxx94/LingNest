#include <QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>

#include "ai/OpenAICompatibleProvider.h"

namespace {

class LocalHttpServer final : public QObject {
public:
    explicit LocalHttpServer(QObject* parent = nullptr)
        : QObject(parent)
    {
        connect(&m_server, &QTcpServer::newConnection, this, [this]() {
            while (QTcpSocket* socket = m_server.nextPendingConnection()) {
                m_buffers.insert(socket, {});
                connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                    QByteArray& buffer = m_buffers[socket];
                    buffer.append(socket->readAll());
                    const int headerEnd = buffer.indexOf("\r\n\r\n");
                    if (headerEnd < 0) {
                        return;
                    }

                    int contentLength = 0;
                    const QList<QByteArray> headerLines = buffer.left(headerEnd).split('\n');
                    for (QByteArray line : headerLines) {
                        line = line.trimmed();
                        if (line.toLower().startsWith("content-length:")) {
                            contentLength = line.mid(line.indexOf(':') + 1).trimmed().toInt();
                        }
                    }
                    if (buffer.size() < headerEnd + 4 + contentLength) {
                        return;
                    }

                    m_lastRequest = buffer;
                    m_requestReceived = true;
                    if (m_respond) {
                        socket->write(m_response);
                        socket->disconnectFromHost();
                    } else if (m_disconnectWithoutResponse) {
                        socket->disconnectFromHost();
                    }
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QObject::destroyed, this, [this, socket]() {
                    m_buffers.remove(socket);
                });
            }
        });
    }

    bool listen()
    {
        return m_server.listen(QHostAddress::LocalHost, 0);
    }

    [[nodiscard]] QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort()));
    }

    void setJsonResponse(int statusCode, const QByteArray& body)
    {
        const QByteArray reason = statusCode == 200 ? QByteArrayLiteral("OK")
                                                    : QByteArrayLiteral("Error");
        m_response = QByteArrayLiteral("HTTP/1.1 ")
            + QByteArray::number(statusCode) + QByteArrayLiteral(" ") + reason
            + QByteArrayLiteral("\r\nContent-Type: application/json\r\nContent-Length: ")
            + QByteArray::number(body.size())
            + QByteArrayLiteral("\r\nConnection: close\r\n\r\n") + body;
        m_respond = true;
        m_disconnectWithoutResponse = false;
        m_requestReceived = false;
        m_lastRequest.clear();
    }

    void setNoResponse()
    {
        m_respond = false;
        m_disconnectWithoutResponse = false;
        m_requestReceived = false;
        m_lastRequest.clear();
    }

    void setDisconnectWithoutResponse()
    {
        m_respond = false;
        m_disconnectWithoutResponse = true;
        m_requestReceived = false;
        m_lastRequest.clear();
    }

    [[nodiscard]] bool requestReceived() const noexcept { return m_requestReceived; }
    [[nodiscard]] QByteArray lastRequest() const { return m_lastRequest; }

private:
    QHash<QTcpSocket*, QByteArray> m_buffers;
    QTcpServer m_server;
    QByteArray m_response;
    QByteArray m_lastRequest;
    bool m_respond {true};
    bool m_disconnectWithoutResponse {false};
    bool m_requestReceived {false};
};

lingnest::ai::OpenAICompatibleSettings settingsFor(
    const QUrl& baseUrl,
    int timeoutMs = 1000)
{
    lingnest::ai::OpenAICompatibleSettings settings;
    settings.baseUrl = baseUrl;
    settings.apiKey = QStringLiteral("test-secret");
    settings.model = QStringLiteral("test-model");
    settings.temperature = 0.25;
    settings.maxTokens = 321;
    settings.timeoutMs = timeoutMs;
    return settings;
}

lingnest::ai::AIChatRequest userRequest()
{
    lingnest::ai::AIChatRequest request;
    request.messages.append(
        {lingnest::ai::ChatRole::System, QStringLiteral("测试提示")});
    request.messages.append(
        {lingnest::ai::ChatRole::User, QStringLiteral("你好")});
    return request;
}

lingnest::ai::AIChatResult takeResult(QSignalSpy& spy, quint64 expectedId)
{
    const QList<QVariant> arguments = spy.takeFirst();
    if (arguments.at(0).toULongLong() != expectedId) {
        return {};
    }
    return qvariant_cast<lingnest::ai::AIChatResult>(arguments.at(1));
}

class OpenAICompatibleProviderTest final : public QObject {
    Q_OBJECT

private slots:
    void sendsExpectedRequestAndParsesSuccess();
    void reportsHttpApiAndJsonErrors();
    void redactsApiKeyEchoedByServer();
    void reportsTimeout();
    void reportsNetworkFailure();
    void cancelsPendingRequest();
    void rejectsMissingConfigurationAsynchronously();
};

void OpenAICompatibleProviderTest::sendsExpectedRequestAndParsesSuccess()
{
    LocalHttpServer server;
    QVERIFY(server.listen());
    server.setJsonResponse(
        200,
        QByteArrayLiteral(
            R"({"choices":[{"message":{"role":"assistant","content":"温柔的回复"}}]})"));

    lingnest::ai::OpenAICompatibleProvider provider(settingsFor(server.baseUrl()));
    QSignalSpy spy(&provider, &lingnest::ai::IAIProvider::chatFinished);
    const quint64 requestId = provider.sendChat(userRequest());

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    const lingnest::ai::AIChatResult result = takeResult(spy, requestId);
    QVERIFY(result.success);
    QCOMPARE(result.content, QStringLiteral("温柔的回复"));
    QCOMPARE(result.errorCode, lingnest::ai::AIErrorCode::None);
    QCOMPARE(result.httpStatus, 200);

    QVERIFY(server.requestReceived());
    const QByteArray rawRequest = server.lastRequest();
    QVERIFY(rawRequest.startsWith("POST /v1/chat/completions HTTP/1.1"));
    QVERIFY(rawRequest.toLower().contains("authorization: bearer test-secret"));
    const int bodyStart = rawRequest.indexOf("\r\n\r\n");
    QVERIFY(bodyStart >= 0);
    const QJsonDocument body = QJsonDocument::fromJson(rawRequest.mid(bodyStart + 4));
    QVERIFY(body.isObject());
    QCOMPARE(body.object().value(QStringLiteral("model")).toString(), QStringLiteral("test-model"));
    QCOMPARE(body.object().value(QStringLiteral("max_tokens")).toInt(), 321);
    QCOMPARE(body.object().value(QStringLiteral("messages")).toArray().size(), 2);
}

void OpenAICompatibleProviderTest::reportsHttpApiAndJsonErrors()
{
    LocalHttpServer server;
    QVERIFY(server.listen());
    lingnest::ai::OpenAICompatibleProvider provider(settingsFor(server.baseUrl()));
    QSignalSpy spy(&provider, &lingnest::ai::IAIProvider::chatFinished);

    server.setJsonResponse(
        401,
        QByteArrayLiteral(R"({"error":{"message":"密钥无效"}})"));
    const quint64 apiRequestId = provider.sendChat(userRequest());
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    const lingnest::ai::AIChatResult apiResult = takeResult(spy, apiRequestId);
    QVERIFY(!apiResult.success);
    QCOMPARE(apiResult.errorCode, lingnest::ai::AIErrorCode::Api);
    QCOMPARE(apiResult.httpStatus, 401);
    QVERIFY(apiResult.errorMessage.contains(QStringLiteral("密钥无效")));

    server.setJsonResponse(200, QByteArrayLiteral("not-json"));
    const quint64 jsonRequestId = provider.sendChat(userRequest());
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    const lingnest::ai::AIChatResult jsonResult = takeResult(spy, jsonRequestId);
    QVERIFY(!jsonResult.success);
    QCOMPARE(jsonResult.errorCode, lingnest::ai::AIErrorCode::Json);
}

void OpenAICompatibleProviderTest::redactsApiKeyEchoedByServer()
{
    LocalHttpServer server;
    QVERIFY(server.listen());
    server.setJsonResponse(
        401,
        QByteArrayLiteral(
            R"({"error":{"message":"credential test-secret was rejected"}})"));

    lingnest::ai::OpenAICompatibleProvider provider(settingsFor(server.baseUrl()));
    QSignalSpy spy(&provider, &lingnest::ai::IAIProvider::chatFinished);
    const quint64 requestId = provider.sendChat(userRequest());

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    const lingnest::ai::AIChatResult result = takeResult(spy, requestId);
    QVERIFY(!result.success);
    QVERIFY(!result.errorMessage.contains(QStringLiteral("test-secret")));
    QVERIFY(result.errorMessage.contains(QStringLiteral("[redacted]")));
}

void OpenAICompatibleProviderTest::reportsTimeout()
{
    LocalHttpServer server;
    QVERIFY(server.listen());
    server.setNoResponse();
    lingnest::ai::OpenAICompatibleProvider timeoutProvider(
        settingsFor(server.baseUrl(), 60));
    QSignalSpy timeoutSpy(
        &timeoutProvider, &lingnest::ai::IAIProvider::chatFinished);
    const quint64 timeoutRequestId = timeoutProvider.sendChat(userRequest());

    QTRY_COMPARE_WITH_TIMEOUT(timeoutSpy.count(), 1, 2000);
    const lingnest::ai::AIChatResult timeoutResult = takeResult(
        timeoutSpy, timeoutRequestId);
    QCOMPARE(timeoutResult.errorCode, lingnest::ai::AIErrorCode::Timeout);
}

void OpenAICompatibleProviderTest::reportsNetworkFailure()
{
    LocalHttpServer server;
    QVERIFY(server.listen());
    server.setDisconnectWithoutResponse();
    lingnest::ai::OpenAICompatibleProvider networkProvider(
        settingsFor(server.baseUrl()));
    QSignalSpy networkSpy(
        &networkProvider, &lingnest::ai::IAIProvider::chatFinished);
    const quint64 networkRequestId = networkProvider.sendChat(userRequest());

    QTRY_COMPARE_WITH_TIMEOUT(networkSpy.count(), 1, 2000);
    const lingnest::ai::AIChatResult networkResult = takeResult(
        networkSpy, networkRequestId);
    QCOMPARE(
        static_cast<int>(networkResult.errorCode),
        static_cast<int>(lingnest::ai::AIErrorCode::Network));
}

void OpenAICompatibleProviderTest::cancelsPendingRequest()
{
    LocalHttpServer server;
    QVERIFY(server.listen());
    server.setNoResponse();
    lingnest::ai::OpenAICompatibleProvider provider(
        settingsFor(server.baseUrl(), 5000));
    QSignalSpy spy(&provider, &lingnest::ai::IAIProvider::chatFinished);

    const quint64 requestId = provider.sendChat(userRequest());
    provider.cancel(requestId);

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
    const lingnest::ai::AIChatResult result = takeResult(spy, requestId);
    QCOMPARE(result.errorCode, lingnest::ai::AIErrorCode::Cancelled);
}

void OpenAICompatibleProviderTest::rejectsMissingConfigurationAsynchronously()
{
    lingnest::ai::OpenAICompatibleSettings settings;
    lingnest::ai::OpenAICompatibleProvider provider(settings);
    QSignalSpy spy(&provider, &lingnest::ai::IAIProvider::chatFinished);

    const quint64 requestId = provider.sendChat(userRequest());
    QCOMPARE(spy.count(), 0);

    QTRY_COMPARE(spy.count(), 1);
    const lingnest::ai::AIChatResult result = takeResult(spy, requestId);
    QCOMPARE(result.errorCode, lingnest::ai::AIErrorCode::Configuration);
}

} // namespace

QTEST_GUILESS_MAIN(OpenAICompatibleProviderTest)

#include "OpenAICompatibleProviderTest.moc"
