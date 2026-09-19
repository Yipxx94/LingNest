#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>

#include "ai/AIChatTypes.h"
#include "character/CharacterDefinition.h"
#include "memory/PromptBuilder.h"
#include "ui/ChatMessageModel.h"

namespace lingnest::ai {
class IAIProvider;
}

namespace lingnest::memory {
class MemoryRepository;
}

namespace lingnest::ui {

class ChatController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* messages READ messages CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QUrl avatarUrl READ avatarUrl CONSTANT)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit ChatController(
        const character::CharacterDefinition& character,
        ai::IAIProvider* provider = nullptr,
        QString systemPrompt = {},
        QObject* parent = nullptr,
        memory::MemoryRepository* memoryRepository = nullptr,
        memory::PromptBuilder* promptBuilder = nullptr);

    [[nodiscard]] QAbstractItemModel* messages();
    [[nodiscard]] QString displayName() const;
    [[nodiscard]] QUrl avatarUrl() const;
    [[nodiscard]] bool isBusy() const noexcept;

    Q_INVOKABLE void sendMessage(const QString& text);
    Q_INVOKABLE void clearChat();

signals:
    void messageAdded();
    void userMessageSubmitted(const QString& text);
    void assistantMessageReady(const QString& text);
    void assistantErrorReady(const QString& text);
    void assistantResponseReady();
    void assistantResponseFailed();
    void busyChanged();

private:
    void restoreConversation();
    void persistConversation(ai::ChatRole role, const QString& content);
    void captureExplicitMemory(const QString& text);
    void trimHistory();
    void handleChatFinished(quint64 requestId, ai::AIChatResult result);
    void setBusy(bool busy);

    ChatMessageModel m_messages;
    ai::IAIProvider* m_provider {nullptr};
    memory::MemoryRepository* m_memoryRepository {nullptr};
    memory::PromptBuilder m_defaultPromptBuilder;
    memory::PromptBuilder* m_promptBuilder {nullptr};
    QVector<ai::AIChatMessage> m_history;
    QString m_characterId;
    QString m_displayName;
    QUrl m_avatarUrl;
    QString m_systemPrompt;
    quint64 m_activeRequestId {0};
    bool m_busy {false};
};

} // namespace lingnest::ui
