#include <QtTest>

#include <QSignalSpy>

#include "animation/IAnimationPlayer.h"
#include "pet/PetStateMachine.h"

namespace {

class FakeAnimationPlayer final : public lingnest::animation::IAnimationPlayer {
public:
    explicit FakeAnimationPlayer(QObject* parent = nullptr)
        : IAnimationPlayer(parent)
    {
    }

    bool play(const QString& animationName, PlaybackMode mode) override
    {
        current = animationName;
        lastMode = mode;
        ++playCount;
        return !animationName.isEmpty();
    }

    void stop() override
    {
    }

    QString currentAnimation() const override
    {
        return current;
    }

    void finishCurrent()
    {
        emit animationFinished(current);
    }

    QString current;
    PlaybackMode lastMode {PlaybackMode::CatalogDefault};
    int playCount {0};
};

lingnest::pet::PetAnimationSet animationSet()
{
    return {
        QStringLiteral("idle"),
        QStringLiteral("blink"),
        QStringLiteral("walk_left"),
        QStringLiteral("walk_right"),
        QStringLiteral("talk"),
        QStringLiteral("happy"),
        QStringLiteral("wave"),
        QStringLiteral("daze_prone"),
        QStringLiteral("sleep"),
        QStringLiteral("thinking")
    };
}

class PetStateMachineTest final : public QObject {
    Q_OBJECT

private slots:
    void clickPlaysHappyOnceThenReturnsIdle();
    void ambientActionsUseTheirOwnAnimations();
    void conversationTransitionsThinkingTalkIdle();
    void walkReturnsIdleAfterDuration();
    void walkDirectionChangeRestartsMatchingAnimation();
    void inactivitySleepsAndActivityWakes();
};

void PetStateMachineTest::clickPlaysHappyOnceThenReturnsIdle()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    machine.start();

    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);
    QCOMPARE(player.current, QStringLiteral("idle"));
    QCOMPARE(player.lastMode, FakeAnimationPlayer::PlaybackMode::Loop);

    machine.userClicked();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Happy);
    QCOMPARE(player.current, QStringLiteral("happy"));
    QCOMPARE(player.lastMode, FakeAnimationPlayer::PlaybackMode::Once);

    player.finishCurrent();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);
    QCOMPARE(player.current, QStringLiteral("idle"));
}

void PetStateMachineTest::ambientActionsUseTheirOwnAnimations()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    machine.start();

    machine.requestBlink();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Blink);
    QCOMPARE(player.current, QStringLiteral("blink"));
    QCOMPARE(player.lastMode, FakeAnimationPlayer::PlaybackMode::Once);
    player.finishCurrent();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);

    machine.requestWave();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Wave);
    QCOMPARE(player.current, QStringLiteral("wave"));
    QCOMPARE(player.lastMode, FakeAnimationPlayer::PlaybackMode::Once);
    player.finishCurrent();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);

    machine.requestDaze(1000);
    QCOMPARE(machine.state(), lingnest::pet::PetState::Daze);
    QCOMPARE(player.current, QStringLiteral("daze_prone"));
    QCOMPARE(player.lastMode, FakeAnimationPlayer::PlaybackMode::Loop);
    machine.interruptForUserActivity();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);
}

void PetStateMachineTest::conversationTransitionsThinkingTalkIdle()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    machine.start();

    machine.beginConversation();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Thinking);
    QCOMPARE(player.current, QStringLiteral("thinking"));
    QCOMPARE(player.lastMode, FakeAnimationPlayer::PlaybackMode::Loop);

    machine.assistantResponseReady();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Talk);
    QCOMPARE(player.current, QStringLiteral("talk"));
    QCOMPARE(player.lastMode, FakeAnimationPlayer::PlaybackMode::Once);

    player.finishCurrent();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);
}

void PetStateMachineTest::walkReturnsIdleAfterDuration()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    machine.start();

    machine.requestWalk(lingnest::pet::WalkDirection::Left, 1);
    QCOMPARE(machine.state(), lingnest::pet::PetState::Walk);
    QCOMPARE(player.current, QStringLiteral("walk_left"));
    QTRY_COMPARE(machine.state(), lingnest::pet::PetState::Idle);
    QCOMPARE(player.current, QStringLiteral("idle"));
}

void PetStateMachineTest::walkDirectionChangeRestartsMatchingAnimation()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    QSignalSpy startedSpy(&machine, &lingnest::pet::PetStateMachine::walkStarted);
    QSignalSpy stoppedSpy(&machine, &lingnest::pet::PetStateMachine::walkStopped);
    QSignalSpy directionSpy(
        &machine, &lingnest::pet::PetStateMachine::walkDirectionChanged);
    machine.start();

    machine.requestWalk(lingnest::pet::WalkDirection::Left, 1000);
    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(player.current, QStringLiteral("walk_left"));

    machine.setWalkDirection(lingnest::pet::WalkDirection::Right);
    QCOMPARE(directionSpy.count(), 1);
    QCOMPARE(machine.walkDirection(), lingnest::pet::WalkDirection::Right);
    QCOMPARE(player.current, QStringLiteral("walk_right"));

    machine.interruptForUserActivity();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);
    QCOMPARE(stoppedSpy.count(), 1);
}

void PetStateMachineTest::inactivitySleepsAndActivityWakes()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    machine.start();

    machine.requestSleep();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Sleep);
    QCOMPARE(player.current, QStringLiteral("sleep"));

    machine.wake();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);
    QCOMPARE(player.current, QStringLiteral("idle"));
}

} // namespace

QTEST_GUILESS_MAIN(PetStateMachineTest)

#include "PetStateMachineTest.moc"
