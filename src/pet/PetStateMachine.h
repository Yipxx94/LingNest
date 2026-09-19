#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "animation/IAnimationPlayer.h"
#include "pet/PetState.h"

namespace lingnest::pet {

struct PetAnimationSet {
    QString idle;
    QString blink;
    QString walkLeft;
    QString walkRight;
    QString talk;
    QString happy;
    QString wave;
    QString daze;
    QString sleep;
    QString thinking;
};

class PetStateMachine final : public QObject {
    Q_OBJECT
    Q_PROPERTY(lingnest::pet::PetState state READ state NOTIFY stateChanged)

public:
    PetStateMachine(
        PetAnimationSet animations,
        animation::IAnimationPlayer* animationPlayer,
        QObject* parent = nullptr);

    [[nodiscard]] PetState state() const noexcept;
    [[nodiscard]] WalkDirection walkDirection() const noexcept;
    [[nodiscard]] static QString stateName(PetState state);

    void start();
    void userClicked();
    void requestBlink();
    void requestWave();
    void requestDaze(int durationMs);
    void beginConversation();
    void assistantResponseReady();
    void requestWalk(WalkDirection direction, int durationMs);
    void setWalkDirection(WalkDirection direction);
    void requestSleep();
    void wake();
    void interruptForUserActivity();
    void forceIdle();

signals:
    void stateChanged(lingnest::pet::PetState state);
    void walkStarted(lingnest::pet::WalkDirection direction);
    void walkStopped();
    void walkDirectionChanged(lingnest::pet::WalkDirection direction);
    void animationError(const QString& message);

private slots:
    void handleAnimationFinished(const QString& animationName);

private:
    void transitionTo(PetState nextState);
    void playCurrentState();
    [[nodiscard]] QString animationForCurrentState() const;

    PetAnimationSet m_animations;
    animation::IAnimationPlayer* m_animationPlayer {nullptr};
    QTimer m_ambientActionTimer;
    PetState m_state {PetState::Idle};
    WalkDirection m_walkDirection {WalkDirection::Right};
    QString m_activeAnimation;
    bool m_started {false};
};

} // namespace lingnest::pet
