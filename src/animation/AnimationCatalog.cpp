#include "animation/AnimationCatalog.h"

#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace lingnest::animation {
namespace {

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

std::optional<QString> resolveFramePath(
    const QString& packageDirectory,
    const QString& relativePath,
    QString* errorMessage)
{
    if (relativePath.isEmpty() || QDir::isAbsolutePath(relativePath)) {
        setError(errorMessage,
            QStringLiteral("Animation frame path must be relative: %1").arg(relativePath));
        return std::nullopt;
    }

    const QString packageCanonical = QFileInfo(packageDirectory).canonicalFilePath();
    const QFileInfo frameInfo(QDir(packageDirectory).filePath(relativePath));
    const QString frameCanonical = frameInfo.canonicalFilePath();
    if (packageCanonical.isEmpty() || frameCanonical.isEmpty() || !frameInfo.isFile()) {
        setError(errorMessage,
            QStringLiteral("Animation frame is missing: %1").arg(relativePath));
        return std::nullopt;
    }

    const QString relativeCanonical = QDir::fromNativeSeparators(
        QDir(packageCanonical).relativeFilePath(frameCanonical));
    if (QDir::isAbsolutePath(relativeCanonical)
        || relativeCanonical == QStringLiteral("..")
        || relativeCanonical.startsWith(QStringLiteral("../"))) {
        setError(errorMessage,
            QStringLiteral("Animation frame escapes the character package: %1")
                .arg(relativePath));
        return std::nullopt;
    }

    return frameCanonical;
}

} // namespace

std::optional<AnimationCatalog> AnimationCatalog::load(
    const QString& manifestPath,
    const QString& packageDirectory,
    QString* errorMessage)
{
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(errorMessage,
            QStringLiteral("Cannot open animation manifest %1: %2")
                .arg(QDir::toNativeSeparators(manifestPath), manifestFile.errorString()));
        return std::nullopt;
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(manifestFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage,
            QStringLiteral("Invalid animation manifest %1: %2")
                .arg(QDir::toNativeSeparators(manifestPath), parseError.errorString()));
        return std::nullopt;
    }

    const QJsonObject actions =
        document.object().value(QStringLiteral("actions")).toObject();
    if (actions.isEmpty()) {
        setError(errorMessage, QStringLiteral("Animation manifest has no actions"));
        return std::nullopt;
    }

    AnimationCatalog catalog;
    for (auto iterator = actions.constBegin(); iterator != actions.constEnd(); ++iterator) {
        const QString animationName = iterator.key();
        const QJsonObject action = iterator.value().toObject();
        const QJsonArray files = action.value(QStringLiteral("files")).toArray();
        const QJsonArray durations = action.value(QStringLiteral("durations_ms")).toArray();
        if (files.isEmpty() || files.size() != durations.size()) {
            setError(errorMessage,
                QStringLiteral("Animation '%1' has mismatched files and durations")
                    .arg(animationName));
            return std::nullopt;
        }

        const QJsonValue declaredFrameCount = action.value(QStringLiteral("frame_count"));
        if (declaredFrameCount.isDouble()
            && declaredFrameCount.toInt() != files.size()) {
            setError(errorMessage,
                QStringLiteral("Animation '%1' frame_count does not match files")
                    .arg(animationName));
            return std::nullopt;
        }

        AnimationClip clip;
        clip.name = animationName;
        clip.loop = action.value(QStringLiteral("loop")).toBool(true);
        clip.loopStartFrame = action.value(QStringLiteral("loop_start_frame")).toInt(0);
        if (clip.loopStartFrame < 0 || clip.loopStartFrame >= files.size()) {
            setError(errorMessage,
                QStringLiteral("Animation '%1' has an invalid loop_start_frame")
                    .arg(animationName));
            return std::nullopt;
        }
        clip.frames.reserve(files.size());
        clip.durationsMs.reserve(durations.size());

        for (int index = 0; index < files.size(); ++index) {
            if (!files.at(index).isString() || !durations.at(index).isDouble()) {
                setError(errorMessage,
                    QStringLiteral("Animation '%1' has an invalid frame entry")
                        .arg(animationName));
                return std::nullopt;
            }

            const int durationMs = durations.at(index).toInt();
            if (durationMs <= 0 || durationMs > 60000) {
                setError(errorMessage,
                    QStringLiteral("Animation '%1' has an invalid frame duration")
                        .arg(animationName));
                return std::nullopt;
            }

            const auto framePath = resolveFramePath(
                packageDirectory, files.at(index).toString(), errorMessage);
            if (!framePath.has_value()) {
                return std::nullopt;
            }

            clip.frames.append(QUrl::fromLocalFile(*framePath));
            clip.durationsMs.append(durationMs);
        }

        catalog.m_clips.insert(animationName, std::move(clip));
    }

    return catalog;
}

const AnimationClip* AnimationCatalog::clip(const QString& name) const
{
    const auto iterator = m_clips.constFind(name);
    return iterator == m_clips.constEnd() ? nullptr : &iterator.value();
}

bool AnimationCatalog::contains(const QString& name) const
{
    return m_clips.contains(name);
}

int AnimationCatalog::size() const noexcept
{
    return m_clips.size();
}

} // namespace lingnest::animation
