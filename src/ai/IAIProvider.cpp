#include "ai/IAIProvider.h"

#include <QMetaType>

namespace lingnest::ai {

IAIProvider::IAIProvider(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<AIChatResult>("lingnest::ai::AIChatResult");
}

} // namespace lingnest::ai
