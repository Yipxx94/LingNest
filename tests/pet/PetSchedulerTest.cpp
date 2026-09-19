#include <QtTest>

#include "animation/IAnimationPlayer.h"
#include "pet/PetScheduler.h"
#include "pet/PetStateMachine.h"

namespace {

class FakeAnimationPlayer final : public lingnest::animation::IAnimationPlayer {
public:
    bool play(const QString& animationName, PlaybackMode mode) override
    {
        current = animationName;
        lastMode = mode;
        return !animationName.isEmpty();
    }

    void stop() override {}
    [[nodiscard]] QString currentAnimation() const override { return current; }

    QString current;
    PlaybackMode lastMode {PlaybackMode::CatalogDefault};
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

void makeOtherTimersSlow(lingnest::pet::PetScheduler& scheduler)
{
    scheduler.setWalkIntervalRangeMs(10000, 10000);
    scheduler.setDazeIntervalRangeMs(10000, 10000);
    scheduler.setBlinkIntervalRangeMs(10000, 10000);
    scheduler.setSleepAfterMs(10000);
}

class PetSchedulerTest final : public QObject {
    Q_OBJECT

private slots:
    void schedulesBlink();
    void schedulesWalk();
    void schedulesDaze();
    void schedulesSleep();
    void activityAndPauseInterruptAmbientActions();
};

void PetSchedulerTest::schedulesBlink()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    makeOtherTimersSlow(scheduler);
    scheduler.setBlinkIntervalRangeMs(10, 10);
    machine.start();
    scheduler.start();

    QTRY_COMPARE(machine.state(), lingnest::pet::PetState::Blink);
    QCOMPARE(player.current, QStringLiteral("blink"));
}

void PetSchedulerTest::schedulesWalk()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    makeOtherTimersSlow(scheduler);
    scheduler.setWalkIntervalRangeMs(10, 10);
    machine.start();
    scheduler.start();

    QTRY_COMPARE(machine.state(), lingnest::pet::PetState::Walk);
    QVERIFY(player.current == QStringLiteral("walk_left")
        || player.current == QStringLiteral("walk_right"));
}

void PetSchedulerTest::schedulesDaze()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    makeOtherTimersSlow(scheduler);
    scheduler.setDazeIntervalRangeMs(10, 10);
    machine.start();
    scheduler.start();

    QTRY_COMPARE(machine.state(), lingnest::pet::PetState::Daze);
    QCOMPARE(player.current, QStringLiteral("daze_prone"));
}

void PetSchedulerTest::schedulesSleep()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    makeOtherTimersSlow(scheduler);
    scheduler.setSleepAfterMs(10);
    machine.start();
    scheduler.start();

    QTRY_COMPARE(machine.state(), lingnest::pet::PetState::Sleep);
    QCOMPARE(player.current, QStringLiteral("sleep"));
}

void PetSchedulerTest::activityAndPauseInterruptAmbientActions()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    makeOtherTimersSlow(scheduler);
    machine.start();

    machine.requestDaze(10000);
    scheduler.notifyUserActivity();
    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);

    machine.requestWalk(lingnest::pet::WalkDirection::Right, 10000);
    scheduler.setEnabled(false);
    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);
    QVERIFY(!scheduler.isEnabled());
}

} // namespace

QTEST_GUILESS_MAIN(PetSchedulerTest)

#include "PetSchedulerTest.moc"
