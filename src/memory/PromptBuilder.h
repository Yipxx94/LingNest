#pragma once

#include <QHash>
#include <QString>
#include <QVector>

#include "ai/AIChatTypes.h"
#include "memory/MemoryTypes.h"

namespace lingnest::memory {

class PromptBuilder final {
public:
    struct Options {
        int maxContextCharacters {12000};
        int maxRecentMessages {12};
        int maxMemories {8};
    };

    PromptBuilder();
    explicit PromptBuilder(Options options);

    [[nodiscard]] ai::AIChatRequest build(
        const QString& characterPrompt,
        const QVector<ai::AIChatMessage>& recentConversation,
        const QVector<MemoryEntry>& memories,
        const QHash<QString, QString>& userProfile,
        const QString& userMessage) const;

private:
    [[nodiscard]] QString buildMemoryContext(
        const QVector<MemoryEntry>& memories,
        const QHash<QString, QString>& userProfile,
        const QString& userMessage) const;

    Options m_options;
};

} // namespace lingnest::memory
