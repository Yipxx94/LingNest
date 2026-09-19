#pragma once

#include <QDateTime>
#include <QString>

#include "ai/AIChatTypes.h"

namespace lingnest::memory {

struct ConversationEntry {
    qint64 id {0};
    ai::ChatRole role {ai::ChatRole::User};
    QString content;
    QDateTime createdAt;
};

struct MemoryEntry {
    qint64 id {0};
    QString type;
    QString key;
    QString content;
    QDateTime createdAt;
    QDateTime updatedAt;
};

} // namespace lingnest::memory
