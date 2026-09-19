#pragma once

#include <QObject>
#include <QtGlobal>

#include "ai/AIChatTypes.h"

namespace lingnest::ai {

class IAIProvider : public QObject {
    Q_OBJECT

public:
    explicit IAIProvider(QObject* parent = nullptr);
    ~IAIProvider() override = default;

    virtual quint64 sendChat(const AIChatRequest& request) = 0;
    virtual void cancel(quint64 requestId) = 0;

signals:
    void chatFinished(quint64 requestId, lingnest::ai::AIChatResult result);
};

} // namespace lingnest::ai
