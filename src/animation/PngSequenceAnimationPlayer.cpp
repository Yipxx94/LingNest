#include "animation/PngSequenceAnimationPlayer.h"

#include <utility>

namespace lingnest::animation {

PngSequenceAnimationPlayer::PngSequenceAnimationPlayer(
    AnimationCatalog catalog,
    QObject* parent)
    : IAnimationPlayer(parent)
    , m_catalog(std::move(catalog))
{
    m_frameTimer.setSingleShot(true);
    connect(&m_frameTimer, &QTimer::timeout,
        this, &PngSequenceAnimationPlayer::advanceFrame);
}

bool PngSequenceAnimationPlayer::play(
    const QString& animationName,
    PlaybackMode mode)
{
    const AnimationClip* requestedClip = m_catalog.clip(animationName);
    if (requestedClip == nullptr || requestedClip->frames.isEmpty()) {
        return false;
    }

    m_frameTimer.stop();
    const bool animationChanged = m_currentAnimation != animationName;
    m_clip = requestedClip;
    m_currentAnimation = animationName;
    m_loop = mode == PlaybackMode::Loop
        || (mode == PlaybackMode::CatalogDefault && requestedClip->loop);
    m_loopStartFrame = requestedClip->loopStartFrame;

    if (animationChanged) {
        emit currentAnimationChanged();
    }

    setPlaying(true);
    showFrame(0);
    return true;
}

void PngSequenceAnimationPlayer::stop()
{
    m_frameTimer.stop();
    setPlaying(false);
}

QString PngSequenceAnimationPlayer::currentAnimation() const
{
    return m_currentAnimation;
}

QUrl PngSequenceAnimationPlayer::currentFrame() const
{
    return m_currentFrame;
}

bool PngSequenceAnimationPlayer::isPlaying() const noexcept
{
    return m_playing;
}

void PngSequenceAnimationPlayer::advanceFrame()
{
    if (!m_playing || m_clip == nullptr) {
        return;
    }

    const int nextFrame = m_frameIndex + 1;
    if (nextFrame < m_clip->frames.size()) {
        showFrame(nextFrame);
        return;
    }

    if (m_loop) {
        showFrame(m_loopStartFrame);
        return;
    }

    const QString finishedAnimation = m_currentAnimation;
    setPlaying(false);
    emit animationFinished(finishedAnimation);
}

void PngSequenceAnimationPlayer::setPlaying(bool playing)
{
    if (m_playing == playing) {
        return;
    }

    m_playing = playing;
    emit playingChanged();
}

void PngSequenceAnimationPlayer::showFrame(int index)
{
    if (m_clip == nullptr || index < 0 || index >= m_clip->frames.size()) {
        return;
    }

    m_frameIndex = index;
    const QUrl nextFrame = m_clip->frames.at(index);
    if (m_currentFrame != nextFrame) {
        m_currentFrame = nextFrame;
        emit currentFrameChanged();
    }

    m_frameTimer.start(m_clip->durationsMs.at(index));
}

} // namespace lingnest::animation
