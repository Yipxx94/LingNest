#pragma once

#include <optional>

#include <QString>

namespace lingnest::config {

class ISecretStore {
public:
    virtual ~ISecretStore() = default;

    [[nodiscard]] virtual std::optional<QString> readSecret(
        const QString& target,
        QString* errorMessage = nullptr) = 0;
    virtual bool writeSecret(
        const QString& target,
        const QString& secret,
        QString* errorMessage = nullptr) = 0;
    virtual bool removeSecret(
        const QString& target,
        QString* errorMessage = nullptr) = 0;
};

} // namespace lingnest::config
