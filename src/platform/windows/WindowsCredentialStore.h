#pragma once

#include "config/ISecretStore.h"

namespace lingnest::platform::windows {

class WindowsCredentialStore final : public config::ISecretStore {
public:
    [[nodiscard]] std::optional<QString> readSecret(
        const QString& target,
        QString* errorMessage = nullptr) override;
    bool writeSecret(
        const QString& target,
        const QString& secret,
        QString* errorMessage = nullptr) override;
    bool removeSecret(
        const QString& target,
        QString* errorMessage = nullptr) override;
};

} // namespace lingnest::platform::windows
