#include "pet/PetScheduler.h"

#include <QRandomGenerator>
#include <QtGlobal>

namespace lingnest::pet {

PetScheduler::PetScheduler(PetStateMachine* stateMachine, QObject* parent)
    : QObject(parent)
    , m_stateMachine(stateMachine)
{
    Q_ASSERT(m_stateMachine != nullptr);

    m_walkTimer.setSingleShot(true);
    m_dazeTimer.setSingleShot(true);
    m_blinkTimer.setSingleShot(true);
    m_sleepTimer.setSingleShot(true);

    connect(&m_walkTimer, &QTimer::timeout,
        this, &PetScheduler::handleWalkTimeout);
    connect(&m_dazeTimer, &QTimer::timeout,
        this, &PetScheduler::handleDazeTimeout);
    connect(&m_blinkTimer, &QTimer::timeout,
        this, &PetScheduler::handleBlinkTimeout);
    connect(&m_sleepTimer, &QTimer::timeout,
        this, &PetScheduler::handleSleepTimeout);
}

bool PetScheduler::isEnabled() const noexcept
{
    return m_enabled;
}

int PetScheduler::minimumIntervalMs() const noexcept
{
    return m_walkMinimumMs;
}

void PetScheduler::start()
{
    if (!m_enabled) {
        return;
    }

    scheduleWalk();
    scheduleDaze();
    scheduleBlink();
    scheduleSleep();
}

void PetScheduler::stop()
{
    m_walkTimer.stop();
    m_dazeTimer.stop();
    m_blinkTimer.stop();
    m_sleepTimer.stop();
}

void PetScheduler::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    if (m_enabled) {
        notifyUserActivity();
    } else {
        stop();
        m_stateMachine->interruptForUserActivity();
    }
    emit enabledChanged();
}

void PetScheduler::setMinimumIntervalMs(int intervalMs)
{
    const int minimumMs = qMax(10, intervalMs);
    setWalkIntervalRangeMs(minimumMs, minimumMs * 2);
}

void PetScheduler::setWalkIntervalRangeMs(int minimumMs, int maximumMs)
{
    m_walkMinimumMs = qMax(10, minimumMs);
    m_walkMaximumMs = qMax(m_walkMinimumMs, maximumMs);
    if (m_enabled && m_walkTimer.isActive()) {
        scheduleWalk();
    }
}

void PetScheduler::setDazeIntervalRangeMs(int minimumMs, int maximumMs)
{
    m_dazeMinimumMs = qMax(10, minimumMs);
    m_dazeMaximumMs = qMax(m_dazeMinimumMs, maximumMs);
    if (m_enabled && m_dazeTimer.isActive()) {
        scheduleDaze();
    }
}

void PetScheduler::setBlinkIntervalRangeMs(int minimumMs, int maximumMs)
{
    m_blinkMinimumMs = qMax(10, minimumMs);
    m_blinkMaximumMs = qMax(m_blinkMinimumMs, maximumMs);
    if (m_enabled && m_blinkTimer.isActive()) {
        scheduleBlink();
    }
}

void PetScheduler::setSleepAfterMs(int intervalMs)
{
    m_sleepAfterMs = qMax(10, intervalMs);
    if (m_enabled && m_sleepTimer.isActive()) {
        scheduleSleep();
    }
}

void PetScheduler::notifyUserActivity()
{
    m_stateMachine->interruptForUserActivity();
    if (!m_enabled) {
        return;
    }

    scheduleWalk();
    scheduleDaze();
    scheduleBlink();
    scheduleSleep();
}

void PetScheduler::handleWalkTimeout()
{
    if (!m_enabled) {
        return;
    }

    if (m_stateMachine->state() == PetState::Idle) {
        const WalkDirection direction = QRandomGenerator::global()->bounded(2) == 0
            ? WalkDirection::Left
            : WalkDirection::Right;
        const int durationMs = randomInterval(5000, 9000);
        m_stateMachine->requestWalk(direction, durationMs);
    }
    scheduleWalk();
}

void PetScheduler::handleDazeTimeout()
{
    if (!m_enabled) {
        return;
    }

    if (m_stateMachine->state() == PetState::Idle) {
        m_stateMachine->requestDaze(randomInterval(8000, 16000));
    }
    scheduleDaze();
}

void PetScheduler::handleBlinkTimeout()
{
    if (!m_enabled) {
        return;
    }

    if (m_stateMachine->state() == PetState::Idle) {
        m_stateMachine->requestBlink();
    }
    scheduleBlink();
}

void PetScheduler::handleSleepTimeout()
{
    if (!m_enabled) {
        return;
    }

    m_stateMachine->requestSleep();
    if (m_stateMachine->state() != PetState::Sleep) {
        scheduleSleep(30000);
    }
}

int PetScheduler::randomInterval(int minimumMs, int maximumMs)
{
    if (maximumMs <= minimumMs) {
        return minimumMs;
    }
    return minimumMs
        + QRandomGenerator::global()->bounded(maximumMs - minimumMs + 1);
}

void PetScheduler::scheduleWalk()
{
    if (m_enabled) {
        m_walkTimer.start(randomInterval(m_walkMinimumMs, m_walkMaximumMs));
    }
}

void PetScheduler::scheduleDaze()
{
    if (m_enabled) {
        m_dazeTimer.start(randomInterval(m_dazeMinimumMs, m_dazeMaximumMs));
    }
}

void PetScheduler::scheduleBlink()
{
    if (m_enabled) {
        m_blinkTimer.start(randomInterval(m_blinkMinimumMs, m_blinkMaximumMs));
    }
}

void PetScheduler::scheduleSleep(int delayMs)
{
    if (m_enabled) {
        m_sleepTimer.start(delayMs >= 0 ? delayMs : m_sleepAfterMs);
    }
}

} // namespace lingnest::pet
