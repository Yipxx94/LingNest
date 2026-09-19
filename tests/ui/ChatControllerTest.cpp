#include <QtTest>

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "ai/IAIProvider.h"
#include "character/CharacterDefinition.h"
#include "memory/MemoryRepository.h"
#include "memory/PromptBuilder.h"
#include "ui/ChatController.h"

namespace {

lingnest::character::CharacterDefinition testCharacter()
{
    lingnest::character::CharacterDefinition character;
    character.id = QStringLiteral("yuai-test");
    character.displayName = QStringLiteral("渝爱");
    return character;
}

class FakeAIProvider final : public lingnest::ai::IAIProvider {
public:
    quint64 sendChat(const lingnest::ai::AIChatRequest& request) override
    {
        lastRequest = request;
        lastRequestId = nextRequestId++;
        return lastRequestId;
    }

    void cancel(quint64 requestId) override
    {
        cancelledRequestId = requestId;
    }

    void succeed(const QString& content)
    {
        lingnest::ai::AIChatResult result;
        result.success = true;
        result.content = content;
        emit chatFinished(lastRequestId, result);
    }

    void fail(const QString& message)
    {
        lingnest::ai::AIChatResult result;
        result.errorCode = lingnest::ai::AIErrorCode::Network;
        result.errorMessage = message;
        emit chatFinished(lastRequestId, result);
    }

    lingnest::ai::AIChatRequest lastRequest;
    quint64 lastRequestId {0};
    quint64 cancelledRequestId {0};
    quint64 nextRequestId {1};
};

class ChatControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void sendsPromptAndCompletesWithoutBlocking();
    void preservesInMemoryConversationContext();
    void reportsProviderFailure();
    void clearCancelsPendingResponse();
    void ignoresBlankMessages();
    void restoresPersistentConversationAndInjectsMemory();
};

void ChatControllerTest::sendsPromptAndCompletesWithoutBlocking()
{
    FakeAIProvider provider;
    lingnest::ui::ChatController controller(
        testCharacter(), &provider, QStringLiteral("你是测试角色。"));
    QAbstractItemModel* messages = controller.messages();
    QSignalSpy userSpy(&controller, &lingnest::ui::ChatController::userMessageSubmitted);
    QSignalSpy responseSpy(&controller, &lingnest::ui::ChatController::assistantResponseReady);
    QSignalSpy messageSpy(&controller, &lingnest::ui::ChatController::assistantMessageReady);

    QCOMPARE(messages->rowCount(), 1);
    controller.sendMessage(QStringLiteral("  你好  "));

    QCOMPARE(messages->rowCount(), 2);
    QVERIFY(controller.isBusy());
    QCOMPARE(userSpy.count(), 1);
    QCOMPARE(userSpy.constFirst().constFirst().toString(), QStringLiteral("你好"));
    QCOMPARE(provider.lastRequest.messages.size(), 2);
    QVERIFY(provider.lastRequest.messages.at(0).role == lingnest::ai::ChatRole::System);
    QCOMPARE(provider.lastRequest.messages.at(0).content, QStringLiteral("你是测试角色。"));
    QVERIFY(provider.lastRequest.messages.at(1).role == lingnest::ai::ChatRole::User);
    QCOMPARE(provider.lastRequest.messages.at(1).content, QStringLiteral("你好"));

    provider.succeed(QStringLiteral("你好，我在。"));

    QCOMPARE(messages->rowCount(), 3);
    QVERIFY(!controller.isBusy());
    QCOMPARE(responseSpy.count(), 1);
    QCOMPARE(messageSpy.count(), 1);
    QCOMPARE(
        messageSpy.constFirst().constFirst().toString(),
        QStringLiteral("你好，我在。"));
    const QModelIndex response = messages->index(2, 0);
    QCOMPARE(
        messages->data(response, Qt::UserRole + 1).toString(),
        QStringLiteral("你好，我在。"));
    QCOMPARE(messages->data(response, Qt::UserRole + 3).toBool(), false);
}

void ChatControllerTest::preservesInMemoryConversationContext()
{
    FakeAIProvider provider;
    lingnest::ui::ChatController controller(
        testCharacter(), &provider, QStringLiteral("角色提示词"));

    controller.sendMessage(QStringLiteral("第一句"));
    provider.succeed(QStringLiteral("第一次回复"));
    controller.sendMessage(QStringLiteral("第二句"));

    QCOMPARE(provider.lastRequest.messages.size(), 4);
    QCOMPARE(provider.lastRequest.messages.at(1).content, QStringLiteral("第一句"));
    QVERIFY(provider.lastRequest.messages.at(2).role == lingnest::ai::ChatRole::Assistant);
    QCOMPARE(provider.lastRequest.messages.at(2).content, QStringLiteral("第一次回复"));
    QCOMPARE(provider.lastRequest.messages.at(3).content, QStringLiteral("第二句"));
}

void ChatControllerTest::reportsProviderFailure()
{
    FakeAIProvider provider;
    lingnest::ui::ChatController controller(testCharacter(), &provider);
    QSignalSpy failureSpy(&controller, &lingnest::ui::ChatController::assistantResponseFailed);
    QSignalSpy errorSpy(&controller, &lingnest::ui::ChatController::assistantErrorReady);

    controller.sendMessage(QStringLiteral("测试失败"));
    provider.fail(QStringLiteral("网络不可用"));

    QVERIFY(!controller.isBusy());
    QCOMPARE(failureSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(
        errorSpy.constFirst().constFirst().toString(),
        QStringLiteral("网络不可用"));
    QCOMPARE(controller.messages()->rowCount(), 3);
    const QModelIndex errorIndex = controller.messages()->index(2, 0);
    QCOMPARE(
        controller.messages()->data(errorIndex, Qt::UserRole + 1).toString(),
        QStringLiteral("网络不可用"));
    QCOMPARE(
        controller.messages()->data(errorIndex, Qt::UserRole + 2).toString(),
        QStringLiteral("系统"));
}

void ChatControllerTest::clearCancelsPendingResponse()
{
    FakeAIProvider provider;
    lingnest::ui::ChatController controller(testCharacter(), &provider);
    QAbstractItemModel* messages = controller.messages();

    controller.sendMessage(QStringLiteral("不要在清空后回复"));
    QVERIFY(controller.isBusy());
    const quint64 requestId = provider.lastRequestId;
    controller.clearChat();

    QCOMPARE(provider.cancelledRequestId, requestId);
    QCOMPARE(messages->rowCount(), 0);
    QVERIFY(!controller.isBusy());
    provider.succeed(QStringLiteral("迟到的回复"));
    QCOMPARE(messages->rowCount(), 0);
}

void ChatControllerTest::ignoresBlankMessages()
{
    FakeAIProvider provider;
    lingnest::ui::ChatController controller(testCharacter(), &provider);
    const int initialCount = controller.messages()->rowCount();

    controller.sendMessage(QStringLiteral("   \n  "));

    QCOMPARE(controller.messages()->rowCount(), initialCount);
    QCOMPARE(provider.lastRequestId, quint64(0));
    QVERIFY(!controller.isBusy());
}

void ChatControllerTest::restoresPersistentConversationAndInjectsMemory()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    lingnest::memory::MemoryRepository repository(
        QDir(temporaryDirectory.path()).filePath(QStringLiteral("memory.sqlite3")));
    QString errorMessage;
    QVERIFY2(repository.open(&errorMessage), qPrintable(errorMessage));
    lingnest::memory::PromptBuilder promptBuilder;

    {
        FakeAIProvider provider;
        lingnest::ui::ChatController controller(
            testCharacter(),
            &provider,
            QStringLiteral("角色提示词"),
            nullptr,
            &repository,
            &promptBuilder);
        controller.sendMessage(QStringLiteral("记住，我喜欢竹子"));
        provider.succeed(QStringLiteral("好，我记住啦。"));
    }

    QCOMPARE(repository.recentConversation(
        QStringLiteral("yuai-test"), 20, &errorMessage).size(), 2);
    QCOMPARE(repository.recentMemories(
        QStringLiteral("yuai-test"), 20, &errorMessage).size(), 1);

    FakeAIProvider restoredProvider;
    lingnest::ui::ChatController restoredController(
        testCharacter(),
        &restoredProvider,
        QStringLiteral("角色提示词"),
        nullptr,
        &repository,
        &promptBuilder);
    QCOMPARE(restoredController.messages()->rowCount(), 2);

    restoredController.sendMessage(QStringLiteral("我之前说过喜欢什么？"));
    QCOMPARE(restoredProvider.lastRequest.messages.size(), 5);
    QVERIFY(restoredProvider.lastRequest.messages.at(1).role
        == lingnest::ai::ChatRole::System);
    QVERIFY(restoredProvider.lastRequest.messages.at(1).content.contains(
        QStringLiteral("我喜欢竹子")));
    QCOMPARE(
        restoredProvider.lastRequest.messages.constLast().content,
        QStringLiteral("我之前说过喜欢什么？"));

    restoredController.clearChat();
    QCOMPARE(repository.recentConversation(
        QStringLiteral("yuai-test"), 20, &errorMessage).size(), 0);
    QCOMPARE(repository.recentMemories(
        QStringLiteral("yuai-test"), 20, &errorMessage).size(), 1);
}

} // namespace

QTEST_GUILESS_MAIN(ChatControllerTest)

#include "ChatControllerTest.moc"
