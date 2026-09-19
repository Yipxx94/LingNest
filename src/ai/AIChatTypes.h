#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

namespace lingnest::ai {

enum class ChatRole {
    System,
    User,
    Assistant
};

struct AIChatMessage {
    ChatRole role {ChatRole::User};
    QString content;
};

struct AIChatRequest {
    QVector<AIChatMessage> messages;
};

enum class AIErrorCode {
    None,
    Cancelled,
    Configuration,
    Timeout,
    Network,
    Http,
    Api,
    Json
};

struct AIChatResult {
    bool success {false};
    QString content;
    AIErrorCode errorCode {AIErrorCode::None};
    QString errorMessage;
    int httpStatus {0};
};

} // namespace lingnest::ai

Q_DECLARE_METATYPE(lingnest::ai::AIChatResult)
