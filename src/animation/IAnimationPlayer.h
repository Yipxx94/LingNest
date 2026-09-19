#pragma once

#include <QObject>
#include <QString>

namespace lingnest::animation {

class IAnimationPlayer : public QObject {
    Q_OBJECT

public:
    enum class PlaybackMode {
        CatalogDefault,
        Loop,
        Once
    };
    Q_ENUM(PlaybackMode)

    explicit IAnimationPlayer(QObject* parent = nullptr);
    ~IAnimationPlayer() override;

    [[nodiscard]] virtual bool play(
        const QString& animationName,
        PlaybackMode mode = PlaybackMode::CatalogDefault) = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual QString currentAnimation() const = 0;

signals:
    void animationFinished(const QString& animationName);
};

} // namespace lingnest::animation

