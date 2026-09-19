#include "pet/PetStateMachine.h"

#include <utility>

#include <QtGlobal>

namespace lingnest::pet {

PetStateMachine::PetStateMachine(
    PetAnimationSet animations,
    animation::IAnimationPlayer* animationPlayer,
    QObject* parent)
    : QObject(parent)
    , m_animations(std::move(animations))
    , m_animationPlayer(animationPlayer)
{
    Q_ASSERT(m_animationPlayer != nullptr);

    m_ambientActionTimer.setSingleShot(true);
    connect(&m_ambientActionTimer, &QTimer::timeout, this, [this]() {
        if (m_state == PetState::Walk || m_state == PetState::Daze) {
            transitionTo(PetState::Idle);
        }
    });
    connect(
        m_animationPlayer,
        &animation::IAnimationPlayer::animationFinished,
        this,
        &PetStateMachine::handleAnimationFinished);
}

PetState PetStateMachine::state() const noexcept
{
    return m_state;
}

WalkDirection PetStateMachine::walkDirection() const noexcept
{
    return m_walkDirection;
}

QString PetStateMachine::stateName(PetState state)
{
    switch (state) {
    case PetState::Idle:
        return QStringLiteral("Idle");
    case PetState::Blink:
        return QStringLiteral("Blink");
    case PetState::Walk:
        return QStringLiteral("Walk");
    case PetState::Talk:
        return QStringLiteral("Talk");
    case PetState::Happy:
        return QStringLiteral("Happy");
    case PetState::Wave:
        return QStringLiteral("Wave");
    case PetState::Daze:
        return QStringLiteral("Daze");
    case PetState::Sleep:
        return QStringLiteral("Sleep");
    case PetState::Thinking:
        return QStringLiteral("Thinking");
    }
    return QStringLiteral("Unknown");
}

void PetStateMachine::start()
{
    if (m_started) {
        return;
    }

    m_started = true;
    playCurrentState();
    emit stateChanged(m_state);
}

void PetStateMachine::userClicked()
{
    transitionTo(PetState::Happy);
}

void PetStateMachine::requestBlink()
{
    if (m_state == PetState::Idle) {
        transitionTo(PetState::Blink);
    }
}

void PetStateMachine::requestWave()
{
    if (m_state == PetState::Idle) {
        transitionTo(PetState::Wave);
    }
}

void PetStateMachine::requestDaze(int durationMs)
{
    if (m_state != PetState::Idle) {
        return;
    }

    transitionTo(PetState::Daze);
    m_ambientActionTimer.start(qMax(250, durationMs));
}

void PetStateMachine::beginConversation()
{
    transitionTo(PetState::Thinking);
}

void PetStateMachine::assistantResponseReady()
{
    if (m_state == PetState::Thinking) {
        transitionTo(PetState::Talk);
    }
}

void PetStateMachine::requestWalk(WalkDirection direction, int durationMs)
{
    if (m_state != PetState::Idle) {
        return;
    }

    m_walkDirection = direction;
    transitionTo(PetState::Walk);
    m_ambientActionTimer.start(qMax(250, durationMs));
}

void PetStateMachine::setWalkDirection(WalkDirection direction)
{
    if (m_state != PetState::Walk || m_walkDirection == direction) {
        return;
    }

    m_walkDirection = direction;
    playCurrentState();
    emit walkDirectionChanged(m_walkDirection);
}

void PetStateMachine::requestSleep()
{
    if (m_state == PetState::Idle || m_state == PetState::Daze) {
        transitionTo(PetState::Sleep);
    }
}

void PetStateMachine::wake()
{
    if (m_state == PetState::Sleep || m_state == PetState::Daze) {
        transitionTo(PetState::Idle);
    }
}

void PetStateMachine::interruptForUserActivity()
{
    switch (m_state) {
    case PetState::Blink:
    case PetState::Walk:
    case PetState::Wave:
    case PetState::Daze:
    case PetState::Sleep:
        transitionTo(PetState::Idle);
        break;
    case PetState::Idle:
    case PetState::Talk:
    case PetState::Happy:
    case PetState::Thinking:
        break;
    }
}

void PetStateMachine::forceIdle()
{
    transitionTo(PetState::Idle);
}

void PetStateMachine::handleAnimationFinished(const QString& animationName)
{
    if (animationName != m_activeAnimation) {
        return;
    }

    if (m_state == PetState::Blink
        || m_state == PetState::Happy
        || m_state == PetState::Talk
        || m_state == PetState::Wave) {
        transitionTo(PetState::Idle);
    }
}

void PetStateMachine::transitionTo(PetState nextState)
{
    if (!m_started) {
        m_started = true;
    }

    if (m_state == nextState) {
        return;
    }

    const bool wasWalking = m_state == PetState::Walk;
    m_ambientActionTimer.stop();
    m_state = nextState;
    playCurrentState();
    emit stateChanged(m_state);

    if (wasWalking && m_state != PetState::Walk) {
        emit walkStopped();
    } else if (!wasWalking && m_state == PetState::Walk) {
        emit walkStarted(m_walkDirection);
    }
}

void PetStateMachine::playCurrentState()
{
    m_activeAnimation = animationForCurrentState();
    const auto playbackMode =
        (m_state == PetState::Blink
            || m_state == PetState::Happy
            || m_state == PetState::Talk
            || m_state == PetState::Wave)
        ? animation::IAnimationPlayer::PlaybackMode::Once
        : animation::IAnimationPlayer::PlaybackMode::Loop;

    if (!m_animationPlayer->play(m_activeAnimation, playbackMode)) {
        emit animationError(
            QStringLiteral("Cannot play animation '%1' for state %2")
                .arg(m_activeAnimation, stateName(m_state)));
    }
}

QString PetStateMachine::animationForCurrentState() const
{
    switch (m_state) {
    case PetState::Idle:
        return m_animations.idle;
    case PetState::Blink:
        return m_animations.blink;
    case PetState::Walk:
        return m_walkDirection == WalkDirection::Left
            ? m_animations.walkLeft
            : m_animations.walkRight;
    case PetState::Talk:
        return m_animations.talk;
    case PetState::Happy:
        return m_animations.happy;
    case PetState::Wave:
        return m_animations.wave;
    case PetState::Daze:
        return m_animations.daze;
    case PetState::Sleep:
        return m_animations.sleep;
    case PetState::Thinking:
        return m_animations.thinking;
    }
    return m_animations.idle;
}

} // namespace lingnest::pet
