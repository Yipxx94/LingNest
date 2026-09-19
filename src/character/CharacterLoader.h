#pragma once

#include <optional>

#include <QString>

#include "character/CharacterDefinition.h"

namespace lingnest::character {

class CharacterLoader final {
public:
    [[nodiscard]] static std::optional<CharacterDefinition> loadFromDirectory(
        const QString& directoryPath,
        QString* errorMessage = nullptr);
};

} // namespace lingnest::character

