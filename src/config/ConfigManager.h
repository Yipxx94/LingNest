#pragma once

#include <optional>

#include <QJsonObject>
#include <QPoint>
#include <QString>

namespace lingnest::config {

struct WindowPlacement {
    QString screenName;
    QPoint position;
    double relativeX {0.0};
    double relativeY {0.0};
    bool hasRelativePosition {false};
};

struct AIProviderConfig {
    QString baseUrl {QStringLiteral("https://api.openai.com/v1")};
    QString model;
    double temperature {0.7};
    int maxTokens {512};
    int timeoutMs {30000};
};

class ConfigManager final {
public:
    explicit ConfigManager(QString filePath = defaultFilePath());

    [[nodiscard]] static QString defaultFilePath();
    [[nodiscard]] const QString& filePath() const noexcept;

    bool load(QString* errorMessage = nullptr);
    bool loadWithRecovery(QString* recoveryMessage = nullptr);
    [[nodiscard]] std::optional<WindowPlacement> petWindowPlacement() const;
    [[nodiscard]] AIProviderConfig aiProviderConfig() const;
    bool savePetWindowPlacement(
        const WindowPlacement& placement,
        QString* errorMessage = nullptr);
    bool saveAIProviderConfig(
        const AIProviderConfig& config,
        QString* errorMessage = nullptr);

private:
    bool save(QString* errorMessage);

    QString m_filePath;
    QJsonObject m_root;
};

} // namespace lingnest::config
