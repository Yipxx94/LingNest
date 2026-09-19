#include "character/CharacterLoader.h"

#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QUrl>

namespace lingnest::character {
namespace {

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

std::optional<QJsonObject> readJsonObject(
    const QString& filePath,
    QString* errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(errorMessage,
            QStringLiteral("Cannot open %1: %2")
                .arg(QDir::toNativeSeparators(filePath), file.errorString()));
        return std::nullopt;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        setError(errorMessage,
            QStringLiteral("Invalid JSON in %1 at offset %2: %3")
                .arg(QDir::toNativeSeparators(filePath))
                .arg(parseError.offset)
                .arg(parseError.errorString()));
        return std::nullopt;
    }

    if (!document.isObject()) {
        setError(errorMessage,
            QStringLiteral("The root value in %1 must be an object")
                .arg(QDir::toNativeSeparators(filePath)));
        return std::nullopt;
    }

    return document.object();
}

std::optional<QString> requiredString(
    const QJsonObject& object,
    const QString& key,
    const QString& sourceName,
    QString* errorMessage)
{
    const QJsonValue value = object.value(key);
    if (!value.isString() || value.toString().trimmed().isEmpty()) {
        setError(errorMessage,
            QStringLiteral("%1 must contain a non-empty string '%2'")
                .arg(sourceName, key));
        return std::nullopt;
    }

    return value.toString().trimmed();
}

std::optional<QString> resolvePackageFile(
    const QString& packageRoot,
    const QString& relativePath,
    const QString& fieldName,
    QString* errorMessage)
{
    if (relativePath.isEmpty() || QDir::isAbsolutePath(relativePath)) {
        setError(errorMessage,
            QStringLiteral("'%1' must be a non-empty relative path").arg(fieldName));
        return std::nullopt;
    }

    const QString rootCanonical = QFileInfo(packageRoot).canonicalFilePath();
    const QFileInfo candidateInfo(QDir(packageRoot).filePath(relativePath));
    const QString candidateCanonical = candidateInfo.canonicalFilePath();

    if (rootCanonical.isEmpty() || candidateCanonical.isEmpty() || !candidateInfo.isFile()) {
        setError(errorMessage,
            QStringLiteral("'%1' references a missing file: %2")
                .arg(fieldName, relativePath));
        return std::nullopt;
    }

    const QString relativeCanonical = QDir::fromNativeSeparators(
        QDir(rootCanonical).relativeFilePath(candidateCanonical));
    if (QDir::isAbsolutePath(relativeCanonical)
        || relativeCanonical == QStringLiteral("..")
        || relativeCanonical.startsWith(QStringLiteral("../"))) {
        setError(errorMessage,
            QStringLiteral("'%1' escapes the character package: %2")
                .arg(fieldName, relativePath));
        return std::nullopt;
    }

    return candidateCanonical;
}

std::optional<QStringList> readStringArray(
    const QJsonObject& object,
    const QString& key,
    const QString& sourceName,
    QString* errorMessage)
{
    const QJsonValue value = object.value(key);
    if (!value.isArray() || value.toArray().isEmpty()) {
        setError(errorMessage,
            QStringLiteral("%1 must contain a non-empty array '%2'")
                .arg(sourceName, key));
        return std::nullopt;
    }

    QStringList values;
    for (const QJsonValue& item : value.toArray()) {
        if (!item.isString() || item.toString().trimmed().isEmpty()) {
            setError(errorMessage,
                QStringLiteral("Every entry in %1.%2 must be a non-empty string")
                    .arg(sourceName, key));
            return std::nullopt;
        }
        values.append(item.toString().trimmed());
    }

    return values;
}

} // namespace

std::optional<CharacterDefinition> CharacterLoader::loadFromDirectory(
    const QString& directoryPath,
    QString* errorMessage)
{
    const QFileInfo packageInfo(directoryPath);
    const QString packageRoot = packageInfo.canonicalFilePath();
    if (packageRoot.isEmpty() || !packageInfo.isDir()) {
        setError(errorMessage,
            QStringLiteral("Character package directory does not exist: %1")
                .arg(QDir::toNativeSeparators(directoryPath)));
        return std::nullopt;
    }

    const QString definitionPath = QDir(packageRoot).filePath(QStringLiteral("character.json"));
    const auto definitionObject = readJsonObject(definitionPath, errorMessage);
    if (!definitionObject.has_value()) {
        return std::nullopt;
    }

    const QString definitionName = QStringLiteral("character.json");
    const auto id = requiredString(*definitionObject, QStringLiteral("id"), definitionName, errorMessage);
    const auto name = requiredString(*definitionObject, QStringLiteral("name"), definitionName, errorMessage);
    const auto displayName = requiredString(
        *definitionObject, QStringLiteral("displayName"), definitionName, errorMessage);
    const auto defaultAnimation = requiredString(
        *definitionObject, QStringLiteral("defaultAnimation"), definitionName, errorMessage);
    const auto avatar = requiredString(
        *definitionObject, QStringLiteral("avatar"), definitionName, errorMessage);
    const auto prompt = requiredString(
        *definitionObject, QStringLiteral("prompt"), definitionName, errorMessage);
    const auto animations = requiredString(
        *definitionObject, QStringLiteral("animations"), definitionName, errorMessage);
    const auto availableAnimations = readStringArray(
        *definitionObject,
        QStringLiteral("availableAnimations"),
        definitionName,
        errorMessage);

    if (!id || !name || !displayName || !defaultAnimation || !avatar || !prompt
        || !animations || !availableAnimations) {
        return std::nullopt;
    }

    static const QRegularExpression validId(QStringLiteral("^[a-z0-9][a-z0-9_-]*$"));
    if (!validId.match(*id).hasMatch()) {
        setError(errorMessage,
            QStringLiteral("Character id contains unsupported characters: %1").arg(*id));
        return std::nullopt;
    }

    if (packageInfo.fileName().compare(*id, Qt::CaseInsensitive) != 0) {
        setError(errorMessage,
            QStringLiteral("Character id '%1' does not match package directory '%2'")
                .arg(*id, packageInfo.fileName()));
        return std::nullopt;
    }

    if (!availableAnimations->contains(*defaultAnimation)) {
        setError(errorMessage,
            QStringLiteral("Default animation '%1' is not listed in availableAnimations")
                .arg(*defaultAnimation));
        return std::nullopt;
    }

    const QJsonObject stateAnimationObject =
        definitionObject->value(QStringLiteral("stateAnimations")).toObject();
    const QStringList requiredStateKeys {
        QStringLiteral("idle"),
        QStringLiteral("blink"),
        QStringLiteral("walkLeft"),
        QStringLiteral("walkRight"),
        QStringLiteral("talk"),
        QStringLiteral("happy"),
        QStringLiteral("wave"),
        QStringLiteral("daze"),
        QStringLiteral("sleep"),
        QStringLiteral("thinking")
    };
    QHash<QString, QString> stateAnimations;
    for (const QString& stateKey : requiredStateKeys) {
        const QJsonValue animationValue = stateAnimationObject.value(stateKey);
        if (!animationValue.isString()
            || !availableAnimations->contains(animationValue.toString())) {
            setError(errorMessage,
                QStringLiteral("stateAnimations.%1 must reference an available animation")
                    .arg(stateKey));
            return std::nullopt;
        }
        stateAnimations.insert(stateKey, animationValue.toString());
    }

    const auto avatarPath = resolvePackageFile(
        packageRoot, *avatar, QStringLiteral("avatar"), errorMessage);
    const auto promptPath = resolvePackageFile(
        packageRoot, *prompt, QStringLiteral("prompt"), errorMessage);
    const auto manifestPath = resolvePackageFile(
        packageRoot, *animations, QStringLiteral("animations"), errorMessage);
    if (!avatarPath || !promptPath || !manifestPath) {
        return std::nullopt;
    }

    const auto manifestObject = readJsonObject(*manifestPath, errorMessage);
    if (!manifestObject.has_value()) {
        return std::nullopt;
    }

    const QJsonObject actions = manifestObject->value(QStringLiteral("actions")).toObject();
    if (actions.isEmpty()) {
        setError(errorMessage, QStringLiteral("Animation manifest must contain an actions object"));
        return std::nullopt;
    }

    QString defaultFramePath;
    for (const QString& animationName : *availableAnimations) {
        const QJsonObject action = actions.value(animationName).toObject();
        const QJsonArray files = action.value(QStringLiteral("files")).toArray();
        const QJsonArray durations = action.value(QStringLiteral("durations_ms")).toArray();

        if (action.isEmpty() || files.isEmpty() || files.size() != durations.size()) {
            setError(errorMessage,
                QStringLiteral("Animation '%1' has invalid files or durations_ms")
                    .arg(animationName));
            return std::nullopt;
        }

        for (const QJsonValue& fileValue : files) {
            if (!fileValue.isString()) {
                setError(errorMessage,
                    QStringLiteral("Animation '%1' contains a non-string file path")
                        .arg(animationName));
                return std::nullopt;
            }

            const auto framePath = resolvePackageFile(
                packageRoot,
                fileValue.toString(),
                QStringLiteral("animations.%1.files").arg(animationName),
                errorMessage);
            if (!framePath) {
                return std::nullopt;
            }

            if (animationName == *defaultAnimation && defaultFramePath.isEmpty()) {
                defaultFramePath = *framePath;
            }
        }
    }

    if (defaultFramePath.isEmpty()) {
        setError(errorMessage,
            QStringLiteral("Default animation '%1' has no usable frame")
                .arg(*defaultAnimation));
        return std::nullopt;
    }

    CharacterDefinition definition;
    definition.id = *id;
    definition.name = *name;
    definition.displayName = *displayName;
    definition.description = definitionObject->value(QStringLiteral("description")).toString();
    definition.availableAnimations = *availableAnimations;
    definition.stateAnimations = std::move(stateAnimations);
    definition.packageDirectory = packageRoot;
    definition.promptFilePath = *promptPath;
    definition.animationManifestPath = *manifestPath;
    definition.defaultAnimation = *defaultAnimation;
    definition.avatarUrl = QUrl::fromLocalFile(*avatarPath);
    definition.defaultFrameUrl = QUrl::fromLocalFile(defaultFramePath);
    definition.scale = definitionObject->value(QStringLiteral("scale")).toDouble(1.0);

    if (definition.scale <= 0.0 || definition.scale > 4.0) {
        setError(errorMessage, QStringLiteral("Character scale must be greater than 0 and at most 4"));
        return std::nullopt;
    }

    const QJsonArray personality = definitionObject->value(QStringLiteral("personality")).toArray();
    for (const QJsonValue& trait : personality) {
        if (trait.isString() && !trait.toString().trimmed().isEmpty()) {
            definition.personality.append(trait.toString().trimmed());
        }
    }

    return definition;
}

} // namespace lingnest::character
