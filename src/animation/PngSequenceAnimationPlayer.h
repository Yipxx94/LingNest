#pragma once

#include <QTimer>
#include <QUrl>

#include "animation/AnimationCatalog.h"
#include "animation/IAnimationPlayer.h"

namespace lingnest::animation {

class PngSequenceAnimationPlayer final : public IAnimationPlayer {
    Q_OBJECT
    Q_PROPERTY(QUrl currentFrame READ currentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(QString currentAnimation READ currentAnimation NOTIFY currentAnimationChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)

public:
    explicit PngSequenceAnimationPlayer(
        AnimationCatalog catalog,
        QObject* parent = nullptr);

    [[nodiscard]] bool play(
        const QString& animationName,
        PlaybackMode mode = PlaybackMode::CatalogDefault) override;
    void stop() override;

    [[nodiscard]] QString currentAnimation() const override;
    [[nodiscard]] QUrl currentFrame() const;
    [[nodiscard]] bool isPlaying() const noexcept;

signals:
    void currentFrameChanged();
    void currentAnimationChanged();
    void playingChanged();

private slots:
    void advanceFrame();

private:
    void setPlaying(bool playing);
    void showFrame(int index);

    AnimationCatalog m_catalog;
    QTimer m_frameTimer;
    const AnimationClip* m_clip {nullptr};
    QString m_currentAnimation;
    QUrl m_currentFrame;
    int m_frameIndex {0};
    int m_loopStartFrame {0};
    bool m_loop {true};
    bool m_playing {false};
};

} // namespace lingnest::animation
