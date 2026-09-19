#include "ui/ChatController.h"

#include <utility>

#include <QDebug>

#include "ai/IAIProvider.h"
#include "memory/ExplicitMemoryParser.h"
#include "memory/MemoryRepository.h"

namespace lingnest::ui {
namespace {

constexpr int restoredConversationLimit = 40;
constexpr int memoryCandidateLimit = 32;
constexpr int maximumInMemoryHistory = 80;

} // namespace

ChatController::ChatController(
    const character::CharacterDefinition& character,
    ai::IAIProvider* provider,
    QString systemPrompt,
    QObject* parent,
    memory::MemoryRepository* memoryRepository,
    memory::PromptBuilder* promptBuilder)
    : QObject(parent)
    , m_messages(this)
    , m_provider(provider)
    , m_memoryRepository(memoryRepository)
    , m_promptBuilder(
          promptBuilder != nullptr ? promptBuilder : &m_defaultPromptBuilder)
    , m_characterId(character.id)
    , m_displayName(character.displayName)
    , m_avatarUrl(character.avatarUrl)
    , m_systemPrompt(std::move(systemPrompt))
{
    if (m_provider != nullptr) {
        connect(
            m_provider,
            &ai::IAIProvider::chatFinished,
            this,
            &ChatController::handleChatFinished);
    }

    restoreConversation();
    if (m_messages.rowCount() == 0) {
        m_messages.append(
            QStringLiteral("你好呀，我是%1。").arg(m_displayName),
            m_displayName,
            false);
    }
}

QAbstractItemModel* ChatController::messages()
{
    return &m_messages;
}

QString ChatController::displayName() const
{
    return m_displayName;
}

QUrl ChatController::avatarUrl() const
{
    return m_avatarUrl;
}

bool ChatController::isBusy() const noexcept
{
    return m_busy;
}

void ChatController::sendMessage(const QString& text)
{
    const QString trimmedText = text.trimmed();
    if (trimmedText.isEmpty() || m_busy) {
        return;
    }

    m_messages.append(trimmedText, QStringLiteral("你"), true);
    emit messageAdded();
    emit userMessageSubmitted(trimmedText);
    persistConversation(ai::ChatRole::User, trimmedText);
    captureExplicitMemory(trimmedText);
    setBusy(true);

    QVector<memory::MemoryEntry> memories;
    QHash<QString, QString> userProfile;
    if (m_memoryRepository != nullptr && m_memoryRepository->isOpen()) {
        QString errorMessage;
        memories = m_memoryRepository->recentMemories(
            m_characterId, memoryCandidateLimit, &errorMessage);
        if (!errorMessage.isEmpty()) {
            qWarning().noquote() << errorMessage;
        }
        errorMessage.clear();
        userProfile = m_memoryRepository->userProfile(
            m_characterId, &errorMessage);
        if (!errorMessage.isEmpty()) {
            qWarning().noquote() << errorMessage;
        }
    }

    const ai::AIChatMessage userMessage {ai::ChatRole::User, trimmedText};
    ai::AIChatRequest request = m_promptBuilder->build(
        m_systemPrompt,
        m_history,
        memories,
        userProfile,
        trimmedText);
    m_history.append(userMessage);
    trimHistory();

    if (m_provider == nullptr) {
        m_messages.append(
            QStringLiteral("AI 服务尚未初始化。"),
            QStringLiteral("系统"),
            false);
        emit messageAdded();
        setBusy(false);
        emit assistantErrorReady(QStringLiteral("AI 服务尚未初始化。"));
        emit assistantResponseFailed();
        return;
    }

    m_activeRequestId = m_provider->sendChat(request);
}

void ChatController::clearChat()
{
    const quint64 requestId = m_activeRequestId;
    m_activeRequestId = 0;
    if (requestId != 0 && m_provider != nullptr) {
        m_provider->cancel(requestId);
    }

    m_history.clear();
    if (m_memoryRepository != nullptr && m_memoryRepository->isOpen()) {
        QString errorMessage;
        if (!m_memoryRepository->clearConversation(
                m_characterId, &errorMessage)) {
            qWarning().noquote() << errorMessage;
        }
    }
    m_messages.clear();
    setBusy(false);
    emit messageAdded();
}

void ChatController::handleChatFinished(
    quint64 requestId,
    ai::AIChatResult result)
{
    if (requestId == 0 || requestId != m_activeRequestId) {
        return;
    }

    m_activeRequestId = 0;
    setBusy(false);
    if (result.success) {
        const QString response = result.content.trimmed();
        m_history.append({ai::ChatRole::Assistant, response});
        trimHistory();
        persistConversation(ai::ChatRole::Assistant, response);
        m_messages.append(response, m_displayName, false);
        emit messageAdded();
        emit assistantMessageReady(response);
        emit assistantResponseReady();
        return;
    }

    const QString message = result.errorMessage.trimmed().isEmpty()
        ? QStringLiteral("AI 回复失败，请稍后重试。")
        : result.errorMessage.trimmed();
    m_messages.append(message, QStringLiteral("系统"), false);
    emit messageAdded();
    emit assistantErrorReady(message);
    emit assistantResponseFailed();
}

void ChatController::restoreConversation()
{
    if (m_memoryRepository == nullptr || !m_memoryRepository->isOpen()) {
        return;
    }

    QString errorMessage;
    const QVector<memory::ConversationEntry> entries =
        m_memoryRepository->recentConversation(
            m_characterId, restoredConversationLimit, &errorMessage);
    if (!errorMessage.isEmpty()) {
        qWarning().noquote() << errorMessage;
        return;
    }

    for (const memory::ConversationEntry& entry : entries) {
        m_history.append({entry.role, entry.content});
        const bool isUser = entry.role == ai::ChatRole::User;
        m_messages.append(
            entry.content,
            isUser ? QStringLiteral("你") : m_displayName,
            isUser);
    }
    trimHistory();
}

void ChatController::persistConversation(
    ai::ChatRole role,
    const QString& content)
{
    if (m_memoryRepository == nullptr || !m_memoryRepository->isOpen()) {
        return;
    }

    QString errorMessage;
    if (!m_memoryRepository->appendConversation(
            m_characterId, role, content, &errorMessage)) {
        qWarning().noquote() << errorMessage;
    }
}

void ChatController::captureExplicitMemory(const QString& text)
{
    if (m_memoryRepository == nullptr || !m_memoryRepository->isOpen()) {
        return;
    }

    const auto explicitMemory = memory::ExplicitMemoryParser::parse(text);
    if (!explicitMemory.has_value()) {
        return;
    }

    QString errorMessage;
    if (!m_memoryRepository->upsertMemory(
            m_characterId,
            explicitMemory->type,
            explicitMemory->key,
            explicitMemory->content,
            &errorMessage)) {
        qWarning().noquote() << errorMessage;
        return;
    }

    if (!explicitMemory->profileKey.isEmpty()) {
        errorMessage.clear();
        if (!m_memoryRepository->upsertUserProfile(
                m_characterId,
                explicitMemory->profileKey,
                explicitMemory->profileValue,
                &errorMessage)) {
            qWarning().noquote() << errorMessage;
        }
    }
}

void ChatController::trimHistory()
{
    const int excess = m_history.size() - maximumInMemoryHistory;
    if (excess > 0) {
        m_history.remove(0, excess);
    }
}

void ChatController::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }

    m_busy = busy;
    emit busyChanged();
}

} // namespace lingnest::ui
