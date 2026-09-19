#include "config/ConfigManager.h"

#include <utility>

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtGlobal>

namespace lingnest::config {
namespace {

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

QJsonObject defaultRoot()
{
    return {{QStringLiteral("schemaVersion"), 2}};
}

} // namespace

ConfigManager::ConfigManager(QString filePath)
    : m_filePath(std::move(filePath))
    , m_root(defaultRoot())
{
}

QString ConfigManager::defaultFilePath()
{
    const QString configDirectory =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(configDirectory).filePath(QStringLiteral("config.json"));
}

const QString& ConfigManager::filePath() const noexcept
{
    return m_filePath;
}

bool ConfigManager::load(QString* errorMessage)
{
    m_root = defaultRoot();

    QFile file(m_filePath);
    if (!file.exists()) {
        return true;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(errorMessage,
            QStringLiteral("Cannot open config file %1: %2")
                .arg(QDir::toNativeSeparators(m_filePath), file.errorString()));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        setError(errorMessage,
            QStringLiteral("Invalid config JSON in %1: %2")
                .arg(QDir::toNativeSeparators(m_filePath), parseError.errorString()));
        return false;
    }

    if (!document.isObject()) {
        setError(errorMessage,
            QStringLiteral("The config root in %1 must be a JSON object")
                .arg(QDir::toNativeSeparators(m_filePath)));
        return false;
    }

    m_root = document.object();
    if (!m_root.contains(QStringLiteral("schemaVersion"))) {
        m_root.insert(QStringLiteral("schemaVersion"), 2);
    }
    return true;
}

bool ConfigManager::loadWithRecovery(QString* recoveryMessage)
{
    if (recoveryMessage != nullptr) {
        recoveryMessage->clear();
    }

    QString loadError;
    if (load(&loadError)) {
        return true;
    }

    if (!QFileInfo::exists(m_filePath)) {
        setError(recoveryMessage, loadError);
        return false;
    }

    const QString timestamp = QDateTime::currentDateTimeUtc().toString(
        QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    const QString backupPath = QStringLiteral("%1.corrupt-%2")
                                   .arg(m_filePath, timestamp);
    if (!QFile::rename(m_filePath, backupPath)) {
        setError(
            recoveryMessage,
            QStringLiteral("%1 The unreadable file could not be backed up.")
                .arg(loadError));
        return false;
    }

    m_root = defaultRoot();
    QString saveError;
    if (!save(&saveError)) {
        setError(
            recoveryMessage,
            QStringLiteral(
                "%1 The unreadable file was preserved as %2, but a clean "
                "configuration could not be created: %3")
                .arg(
                    loadError,
                    QDir::toNativeSeparators(backupPath),
                    saveError));
        return false;
    }

    setError(
        recoveryMessage,
        QStringLiteral(
            "The invalid configuration was reset. A backup was preserved as %1.")
            .arg(QDir::toNativeSeparators(backupPath)));
    return true;
}

AIProviderConfig ConfigManager::aiProviderConfig() const
{
    AIProviderConfig config;
    const QJsonObject aiObject = m_root.value(QStringLiteral("ai")).toObject();

    const QString baseUrl = aiObject.value(QStringLiteral("baseUrl")).toString().trimmed();
    if (!baseUrl.isEmpty()) {
        config.baseUrl = baseUrl;
    }

    config.model = aiObject.value(QStringLiteral("model")).toString().trimmed();

    const QJsonValue temperature = aiObject.value(QStringLiteral("temperature"));
    if (temperature.isDouble()) {
        config.temperature = qBound(0.0, temperature.toDouble(), 2.0);
    }

    const QJsonValue maxTokens = aiObject.value(QStringLiteral("maxTokens"));
    if (maxTokens.isDouble() && maxTokens.toInt() > 0) {
        config.maxTokens = maxTokens.toInt();
    }

    const QJsonValue timeoutMs = aiObject.value(QStringLiteral("timeoutMs"));
    if (timeoutMs.isDouble() && timeoutMs.toInt() > 0) {
        config.timeoutMs = timeoutMs.toInt();
    }
    return config;
}

std::optional<WindowPlacement> ConfigManager::petWindowPlacement() const
{
    const QJsonObject windowObject = m_root.value(QStringLiteral("window")).toObject();
    const QJsonObject petObject = windowObject.value(QStringLiteral("pet")).toObject();

    if (!petObject.value(QStringLiteral("x")).isDouble()
        || !petObject.value(QStringLiteral("y")).isDouble()) {
        return std::nullopt;
    }

    WindowPlacement placement;
    placement.screenName = petObject.value(QStringLiteral("screen")).toString();
    placement.position = QPoint(
        petObject.value(QStringLiteral("x")).toInt(),
        petObject.value(QStringLiteral("y")).toInt());

    const QJsonValue relativeX = petObject.value(QStringLiteral("relativeX"));
    const QJsonValue relativeY = petObject.value(QStringLiteral("relativeY"));
    if (relativeX.isDouble() && relativeY.isDouble()) {
        placement.relativeX = qBound(0.0, relativeX.toDouble(), 1.0);
        placement.relativeY = qBound(0.0, relativeY.toDouble(), 1.0);
        placement.hasRelativePosition = true;
    }

    return placement;
}

bool ConfigManager::savePetWindowPlacement(
    const WindowPlacement& placement,
    QString* errorMessage)
{
    QJsonObject petObject;
    petObject.insert(QStringLiteral("screen"), placement.screenName);
    petObject.insert(QStringLiteral("x"), placement.position.x());
    petObject.insert(QStringLiteral("y"), placement.position.y());
    petObject.insert(QStringLiteral("relativeX"), qBound(0.0, placement.relativeX, 1.0));
    petObject.insert(QStringLiteral("relativeY"), qBound(0.0, placement.relativeY, 1.0));

    QJsonObject windowObject = m_root.value(QStringLiteral("window")).toObject();
    windowObject.insert(QStringLiteral("pet"), petObject);
    m_root.insert(QStringLiteral("window"), windowObject);

    return save(errorMessage);
}

bool ConfigManager::saveAIProviderConfig(
    const AIProviderConfig& config,
    QString* errorMessage)
{
    QJsonObject aiObject;
    aiObject.insert(QStringLiteral("provider"), QStringLiteral("openai-compatible"));
    aiObject.insert(QStringLiteral("baseUrl"), config.baseUrl.trimmed());
    aiObject.insert(QStringLiteral("model"), config.model.trimmed());
    aiObject.insert(
        QStringLiteral("temperature"),
        qBound(0.0, config.temperature, 2.0));
    aiObject.insert(QStringLiteral("maxTokens"), qMax(1, config.maxTokens));
    aiObject.insert(QStringLiteral("timeoutMs"), qMax(1, config.timeoutMs));
    m_root.insert(QStringLiteral("ai"), aiObject);
    m_root.insert(QStringLiteral("schemaVersion"), 2);

    return save(errorMessage);
}

bool ConfigManager::save(QString* errorMessage)
{
    const QFileInfo configInfo(m_filePath);
    QDir parentDirectory = configInfo.dir();
    if (!parentDirectory.exists() && !parentDirectory.mkpath(QStringLiteral("."))) {
        setError(errorMessage,
            QStringLiteral("Cannot create config directory: %1")
                .arg(QDir::toNativeSeparators(parentDirectory.absolutePath())));
        return false;
    }

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setError(errorMessage,
            QStringLiteral("Cannot write config file %1: %2")
                .arg(QDir::toNativeSeparators(m_filePath), file.errorString()));
        return false;
    }

    const QByteArray data = QJsonDocument(m_root).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        setError(errorMessage,
            QStringLiteral("Cannot fully write config file %1: %2")
                .arg(QDir::toNativeSeparators(m_filePath), file.errorString()));
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        setError(errorMessage,
            QStringLiteral("Cannot commit config file %1: %2")
                .arg(QDir::toNativeSeparators(m_filePath), file.errorString()));
        return false;
    }

    return true;
}

} // namespace lingnest::config
