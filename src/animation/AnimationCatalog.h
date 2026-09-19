#pragma once

#include <optional>

#include <QHash>
#include <QString>
#include <QUrl>
#include <QVector>

namespace lingnest::animation {

struct AnimationClip {
    QString name;
    QVector<QUrl> frames;
    QVector<int> durationsMs;
    bool loop {true};
    int loopStartFrame {0};
};

class AnimationCatalog final {
public:
    [[nodiscard]] static std::optional<AnimationCatalog> load(
        const QString& manifestPath,
        const QString& packageDirectory,
        QString* errorMessage = nullptr);

    [[nodiscard]] const AnimationClip* clip(const QString& name) const;
    [[nodiscard]] bool contains(const QString& name) const;
    [[nodiscard]] int size() const noexcept;

private:
    QHash<QString, AnimationClip> m_clips;
};

} // namespace lingnest::animation
