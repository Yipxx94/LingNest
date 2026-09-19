#include "interaction/InteractionController.h"

#include <QGuiApplication>
#include <QRandomGenerator>
#include <QStringList>
#include <QStyleHints>
#include <QtGlobal>

#include "pet/PetScheduler.h"
#include "pet/PetStateMachine.h"
#include "ui/SpeechBubbleController.h"

namespace lingnest::interaction {

InteractionController::InteractionController(
    pet::PetStateMachine* stateMachine,
    pet::PetScheduler* scheduler,
    ui::SpeechBubbleController* bubbleController,
    QObject* parent)
    : QObject(parent)
    , m_stateMachine(stateMachine)
    , m_scheduler(scheduler)
    , m_bubbleController(bubbleController)
{
    Q_ASSERT(m_stateMachine != nullptr);
    Q_ASSERT(m_scheduler != nullptr);
    Q_ASSERT(m_bubbleController != nullptr);

    m_singleClickTimer.setSingleShot(true);
    m_singleClickTimer.setInterval(
        QGuiApplication::styleHints()->mouseDoubleClickInterval());
    connect(&m_singleClickTimer, &QTimer::timeout,
        this, &InteractionController::commitSingleClick);
}

bool InteractionController::isPaused() const noexcept
{
    return m_paused;
}

bool InteractionController::isChatVisible() const noexcept
{
    return m_chatVisible;
}

bool InteractionController::isSettingsVisible() const noexcept
{
    return m_settingsVisible;
}

void InteractionController::setClickDelayMs(int delayMs)
{
    m_singleClickTimer.setInterval(qMax(1, delayMs));
}

void InteractionController::clickCandidate()
{
    if (m_singleClickTimer.isActive()) {
        doubleClick();
        return;
    }
    m_singleClickTimer.start();
}

void InteractionController::doubleClick()
{
    m_singleClickTimer.stop();
    m_scheduler->notifyUserActivity();
    openChat();
}

void InteractionController::quickResponse()
{
    m_singleClickTimer.stop();
    commitSingleClick();
}

void InteractionController::openChat()
{
    m_scheduler->notifyUserActivity();
    m_bubbleController->hide();
    closeSettings();
    if (m_chatVisible) {
        return;
    }

    m_chatVisible = true;
    emit chatVisibleChanged();
}

void InteractionController::closeChat()
{
    if (!m_chatVisible) {
        return;
    }

    m_chatVisible = false;
    emit chatVisibleChanged();
}

void InteractionController::showSettings()
{
    m_scheduler->notifyUserActivity();
    m_bubbleController->hide();
    closeChat();
    if (m_settingsVisible) {
        return;
    }
    m_settingsVisible = true;
    emit settingsVisibleChanged();
}

void InteractionController::closeSettings()
{
    if (!m_settingsVisible) {
        return;
    }
    m_settingsVisible = false;
    emit settingsVisibleChanged();
}

void InteractionController::showCharacterSwitcher()
{
    m_scheduler->notifyUserActivity();
    m_bubbleController->showMessage(
        QStringLiteral("现在只有渝爱住在这里。"));
}

void InteractionController::togglePaused()
{
    m_paused = !m_paused;
    m_scheduler->setEnabled(!m_paused);
    emit pausedChanged();
    m_bubbleController->showMessage(
        m_paused
            ? QStringLiteral("我先安静待一会儿。")
            : QStringLiteral("好呀，我又可以活动啦。"));
}

void InteractionController::playDebugAction(const QString& actionName)
{
    m_singleClickTimer.stop();
    m_scheduler->notifyUserActivity();
    m_bubbleController->hide();

    if (actionName == QStringLiteral("idle")) {
        m_stateMachine->forceIdle();
    } else if (actionName == QStringLiteral("blink")) {
        m_stateMachine->requestBlink();
    } else if (actionName == QStringLiteral("happy")) {
        m_stateMachine->userClicked();
    } else if (actionName == QStringLiteral("wave")) {
        m_stateMachine->requestWave();
    } else if (actionName == QStringLiteral("daze")) {
        m_stateMachine->requestDaze(12000);
    } else if (actionName == QStringLiteral("walkLeft")) {
        m_stateMachine->requestWalk(pet::WalkDirection::Left, 8000);
    } else if (actionName == QStringLiteral("walkRight")) {
        m_stateMachine->requestWalk(pet::WalkDirection::Right, 8000);
    } else if (actionName == QStringLiteral("sleep")) {
        m_stateMachine->requestSleep();
    } else {
        m_bubbleController->showMessage(
            QStringLiteral("未知动作：%1").arg(actionName));
    }
}

void InteractionController::quitApplication()
{
    emit quitRequested();
}

void InteractionController::commitSingleClick()
{
    static const QStringList messages {
        QStringLiteral("我在呢。"),
        QStringLiteral("今天也辛苦啦。"),
        QStringLiteral("嘿，被你发现了。"),
        QStringLiteral("要记得偶尔休息一下。")
    };

    m_scheduler->notifyUserActivity();
    m_stateMachine->userClicked();
    const int index = QRandomGenerator::global()->bounded(messages.size());
    m_bubbleController->showMessage(messages.at(index));
}

} // namespace lingnest::interaction
