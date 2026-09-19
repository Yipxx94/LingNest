#include <QtTest>

#include <QSignalSpy>

#include "animation/IAnimationPlayer.h"
#include "interaction/InteractionController.h"
#include "pet/PetScheduler.h"
#include "pet/PetStateMachine.h"
#include "ui/SpeechBubbleController.h"

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

class InteractionControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void singleClickShowsLocalFeedback();
    void doubleClickSuppressesSingleClickAndOpensChat();
    void quickResponseBypassesDoubleClickArbitration();
    void debugActionsReachPreviouslyUnwiredStates();
    void settingsAndChatWindowsAreMutuallyExclusive();
    void pauseAndQuitActionsAreReliable();
};

void InteractionControllerTest::singleClickShowsLocalFeedback()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    lingnest::ui::SpeechBubbleController bubble;
    lingnest::interaction::InteractionController controller(
        &machine, &scheduler, &bubble);
    controller.setClickDelayMs(10);
    machine.start();

    controller.clickCandidate();

    QTRY_COMPARE(machine.state(), lingnest::pet::PetState::Happy);
    QVERIFY(bubble.isVisible());
    QVERIFY(!bubble.text().isEmpty());
    QVERIFY(!bubble.isThinking());
    QVERIFY(!controller.isChatVisible());
}

void InteractionControllerTest::settingsAndChatWindowsAreMutuallyExclusive()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    lingnest::ui::SpeechBubbleController bubble;
    lingnest::interaction::InteractionController controller(
        &machine, &scheduler, &bubble);

    controller.showSettings();
    QVERIFY(controller.isSettingsVisible());
    QVERIFY(!controller.isChatVisible());

    controller.openChat();
    QVERIFY(controller.isChatVisible());
    QVERIFY(!controller.isSettingsVisible());

    controller.showSettings();
    QVERIFY(controller.isSettingsVisible());
    QVERIFY(!controller.isChatVisible());
    controller.closeSettings();
    QVERIFY(!controller.isSettingsVisible());
}

void InteractionControllerTest::doubleClickSuppressesSingleClickAndOpensChat()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    lingnest::ui::SpeechBubbleController bubble;
    lingnest::interaction::InteractionController controller(
        &machine, &scheduler, &bubble);
    controller.setClickDelayMs(80);
    machine.start();

    controller.clickCandidate();
    controller.doubleClick();
    QTest::qWait(120);

    QCOMPARE(machine.state(), lingnest::pet::PetState::Idle);
    QVERIFY(controller.isChatVisible());
    QVERIFY(!bubble.isVisible());
}

void InteractionControllerTest::quickResponseBypassesDoubleClickArbitration()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    lingnest::ui::SpeechBubbleController bubble;
    lingnest::interaction::InteractionController controller(
        &machine, &scheduler, &bubble);
    controller.setClickDelayMs(80);
    machine.start();

    controller.clickCandidate();
    controller.quickResponse();
    QTest::qWait(120);

    QCOMPARE(machine.state(), lingnest::pet::PetState::Happy);
    QVERIFY(bubble.isVisible());
    QVERIFY(!bubble.text().isEmpty());
    QVERIFY(!controller.isChatVisible());
}

void InteractionControllerTest::debugActionsReachPreviouslyUnwiredStates()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    lingnest::ui::SpeechBubbleController bubble;
    lingnest::interaction::InteractionController controller(
        &machine, &scheduler, &bubble);
    machine.start();

    controller.playDebugAction(QStringLiteral("daze"));
    QCOMPARE(machine.state(), lingnest::pet::PetState::Daze);
    QCOMPARE(player.current, QStringLiteral("daze_prone"));

    controller.playDebugAction(QStringLiteral("walkLeft"));
    QCOMPARE(machine.state(), lingnest::pet::PetState::Walk);
    QCOMPARE(player.current, QStringLiteral("walk_left"));

    controller.playDebugAction(QStringLiteral("sleep"));
    QCOMPARE(machine.state(), lingnest::pet::PetState::Sleep);
    QCOMPARE(player.current, QStringLiteral("sleep"));

    controller.playDebugAction(QStringLiteral("wave"));
    QCOMPARE(machine.state(), lingnest::pet::PetState::Wave);
    QCOMPARE(player.current, QStringLiteral("wave"));
}

void InteractionControllerTest::pauseAndQuitActionsAreReliable()
{
    FakeAnimationPlayer player;
    lingnest::pet::PetStateMachine machine(animationSet(), &player);
    lingnest::pet::PetScheduler scheduler(&machine);
    lingnest::ui::SpeechBubbleController bubble;
    lingnest::interaction::InteractionController controller(
        &machine, &scheduler, &bubble);
    QSignalSpy pauseSpy(&controller, &lingnest::interaction::InteractionController::pausedChanged);
    QSignalSpy quitSpy(&controller, &lingnest::interaction::InteractionController::quitRequested);

    controller.togglePaused();
    QVERIFY(controller.isPaused());
    QVERIFY(!scheduler.isEnabled());
    QCOMPARE(pauseSpy.count(), 1);

    controller.togglePaused();
    QVERIFY(!controller.isPaused());
    QVERIFY(scheduler.isEnabled());
    QCOMPARE(pauseSpy.count(), 2);

    controller.quitApplication();
    QCOMPARE(quitSpy.count(), 1);
}

} // namespace

QTEST_MAIN(InteractionControllerTest)

#include "InteractionControllerTest.moc"
