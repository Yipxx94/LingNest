#pragma once

#include <QObject>
#include <QTimer>

namespace lingnest::pet {
class PetScheduler;
class PetStateMachine;
}

namespace lingnest::ui {
class SpeechBubbleController;
}

namespace lingnest::interaction {

class InteractionController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool paused READ isPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool chatVisible READ isChatVisible NOTIFY chatVisibleChanged)
    Q_PROPERTY(bool settingsVisible READ isSettingsVisible NOTIFY settingsVisibleChanged)

public:
    InteractionController(
        pet::PetStateMachine* stateMachine,
        pet::PetScheduler* scheduler,
        ui::SpeechBubbleController* bubbleController,
        QObject* parent = nullptr);

    [[nodiscard]] bool isPaused() const noexcept;
    [[nodiscard]] bool isChatVisible() const noexcept;
    [[nodiscard]] bool isSettingsVisible() const noexcept;
    void setClickDelayMs(int delayMs);

    Q_INVOKABLE void clickCandidate();
    Q_INVOKABLE void doubleClick();
    Q_INVOKABLE void quickResponse();
    Q_INVOKABLE void openChat();
    Q_INVOKABLE void closeChat();
    Q_INVOKABLE void showSettings();
    Q_INVOKABLE void closeSettings();
    Q_INVOKABLE void showCharacterSwitcher();
    Q_INVOKABLE void togglePaused();
    Q_INVOKABLE void playDebugAction(const QString& actionName);
    Q_INVOKABLE void quitApplication();

signals:
    void pausedChanged();
    void chatVisibleChanged();
    void settingsVisibleChanged();
    void quitRequested();

private slots:
    void commitSingleClick();

private:
    pet::PetStateMachine* m_stateMachine {nullptr};
    pet::PetScheduler* m_scheduler {nullptr};
    ui::SpeechBubbleController* m_bubbleController {nullptr};
    QTimer m_singleClickTimer;
    bool m_paused {false};
    bool m_chatVisible {false};
    bool m_settingsVisible {false};
};

} // namespace lingnest::interaction
