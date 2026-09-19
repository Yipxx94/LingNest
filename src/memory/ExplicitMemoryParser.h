#pragma once

#include <optional>

#include <QString>

namespace lingnest::memory {

struct ExplicitMemory {
    QString type;
    QString key;
    QString content;
    QString profileKey;
    QString profileValue;
};

class ExplicitMemoryParser final {
public:
    [[nodiscard]] static std::optional<ExplicitMemory> parse(
        const QString& userMessage);
};

} // namespace lingnest::memory
