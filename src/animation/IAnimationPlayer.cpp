#include "animation/IAnimationPlayer.h"

namespace lingnest::animation {

IAnimationPlayer::IAnimationPlayer(QObject* parent)
    : QObject(parent)
{
}

IAnimationPlayer::~IAnimationPlayer() = default;

} // namespace lingnest::animation

